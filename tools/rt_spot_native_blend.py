"""Preserve native PCF-only spotlight fade when replacing its visibility with RT.

The mask is the native shader's layer/type encoding: bit 1 is a separate static
VSM layer; bits 8/9 select the other PCF/VSM layer. Only PCF without that separate
static layer is handled here. Do not fade combined static/dynamic RT visibility
for other flag combinations. No light ID, room coordinate or fixed blend value.
"""
import hashlib
from dxbc_transform import transform_shex
from rt_spot_history import dest, source

BASE_SHA='55bcdf543e60dee904c656bfed355aa1f10393a8f0a950467f25a9ed5ee76a1c'
LAYER_MASK=0x302
PCF_ONLY=0x100


def native_blend(data):
    if hashlib.sha256(data).hexdigest()!=BASE_SHA:
        raise ValueError('Unsupported corrected spotlight composition')
    def patch(w):
        assert w[170]&2047==104 and w[171]==22
        assert w[1716]&2047==57
        w[171]=23
        # Existing r4.w is record byte 304 (flags), r11.y is byte 200 (fade).
        # These are live here inside the native selected-light projection branch.
        # r7.z is the filtered RT spot visibility; r7.w is the separate point.
        extra=[0x07000001,*dest(22,1),*source(4,'wwww'),0x4001,LAYER_MASK]
        extra += [0x07000020,*dest(22,1),*source(22,'xxxx'),0x4001,PCF_ONLY]
        extra += [0x07000000,*dest(22,2),*source(7,'zzzz'),0x4001,0xbf800000]
        extra += [0x09000032,*dest(22,2),*source(11,'yyyy'),*source(22,'yyyy'),0x4001,0x3f800000]
        extra += [0x09000037,*dest(7,4),*source(22,'xxxx'),*source(22,'yyyy'),*source(7,'zzzz')]
        w[1716:1716]=extra
    return transform_shex(data,patch)


def visibility(flags,fade,rt):
    return 1+fade*(rt-1) if flags & LAYER_MASK == PCF_ONLY else rt


def native_map_guard(data):
    """R200: preserve the native selected-spot map as an occlusion ceiling.

    Save the native final static/dynamic selection before its RT bypass. Keep
    the bypass (r0.x=1) for material evaluation, then apply min once to r7.z.
    This avoids multiplying two penumbras or applying the authored fade twice.
    """
    baseline = native_blend(data)
    def patch(w):
        assert w[1707:1716] == [0x05000036,0x00100012,0,0x00004001,0x3f800000]+[0x0100003a]*4
        assert w[1755]&2047 == 57
        # The exact original native movc, redirected to the unused r22.z lane.
        save=[0x09000037,*dest(22,4),0x0010003a,1,0x0010003a,6,0x0010000a,0]
        guard=[0x07000033,*dest(7,4),*source(7,'zzzz'),*source(22,'zzzz')]
        w[1755:1755]=guard
        w[1707:1707]=save
    return transform_shex(baseline,patch)


def guarded_visibility(flags,fade,rt,native_map):
    return min(visibility(flags,fade,rt),native_map)
