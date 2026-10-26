"""Rank spotlight candidates before map shadowing; retain mapped nonselected light.

Hash-locked to the captured direct-light/ID producer. Rays and selected-light
composition are unchanged. Material response, frustum and cookie tests remain.
"""
import hashlib
from dxbc_transform import transform_shex
from rt_spot_history import instruction, dest, source, immediate

NATIVE_SHA = 'a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f'

def multiply(dst, src, weight):
    body=dest(dst,7)+source(src)+source(weight,'xxxx')
    return [((len(body)+1)<<24)|56,*body]

def unshadowed_selection(data):
    if hashlib.sha256(data).hexdigest()!=NATIVE_SHA:
        raise ValueError('Unsupported spotlight selector')
    def transform(w):
        assert w[225]&2047==104 and w[226]==43
        assert w[5685]&2047==48 and w[7313]&2047==57
        assert w[7596]&2047==56 and w[10445]&2047==0
        assert w[10453]&2047==49
        # Extra register is private to the spotlight loop. Initialize it even
        # when the native frustum branch rejects this candidate.
        w[226]=44
        edits={
            5686:(0,instruction('mov',dest(43,1),immediate([0,0,0,0]))),
            # Preserve final static/dynamic map visibility for native accumulation.
            7313:(7,instruction('mov',dest(43,1),source(28,'wwww'))+
                instruction('mov',dest(29,8),immediate([0xffffffff]*4))),
            # Keep the native near-plane mask, omit multiplication by map visibility.
            7596:(7,[0x0100003a]*7),
            # Score this candidate's unshadowed diffuse+specular contribution.
            # Do not use the old adjacent-candidate brightness difference.
            10445:(8,instruction('mov',dest(33,1),source(13,'wwww'))),
            # Native selection/accumulation below still receives mapped lighting.
            10453:(0,multiply(36,36,43)+multiply(35,35,43)),
        }
        for at,(size,replacement) in sorted(edits.items(),reverse=True):
            w[at:at+size]=replacement
    return transform_shex(data,transform)


CORRECTED_SELECTOR_SHA = '29177f00d24b7c7bc9a9563bb04de4c62f16838a198b1275342bddafc3770bc9'

def map_priority_selection(data):
    """Rank by mapped candidate energy; preserve all accumulation and normals."""
    if hashlib.sha256(data).hexdigest()!=CORRECTED_SELECTOR_SHA:
        raise ValueError('Unsupported corrected spotlight selector')
    def transform(words):
        expected=[0x05000036,*dest(33,1),*source(13,'wwww')]
        if words[10459:10464]!=expected:
            raise ValueError('Corrected candidate score anchor changed')
        # r13.w is this candidate's unshadowed energy; r43.x is its saved
        # final map visibility. Do not restore native adjacent-score ranking.
        words[10459:10464]=[0x07000038,*dest(33,1),*source(13,'wwww'),*source(43,'xxxx')]
    return transform_shex(data,transform)


def map_visibility_probe(data):
    """Diagnostic only: encode the winning spotlight's native map visibility.

    Preserve ranking, IDs and every atlas sample. A private register follows
    the same winner predicate as r34.xy. After native output dithering, replace
    diffuse RGB with (visibility, 1-visibility, selected ID + 1). The existing
    composition-before capture reads this payload, not ordinary radiance.
    """
    if hashlib.sha256(data).hexdigest()!=CORRECTED_SELECTOR_SHA:
        raise ValueError('Unsupported map probe selector')
    def transform(w):
        assert w[225]&2047==104 and w[226]==44
        assert w[10549]&2047==55 and w[14948]&2047==164
        w[226]=45
        # Zero means no selected candidate; z is a separate validity/ID lane.
        init=instruction('mov',dest(44,15),immediate([0,0,0,0]))
        # movc r44.x, r14.w, r43.x, r44.x: exact existing winner predicate.
        winner=[0x09000037,*dest(44,1),*source(14,'wwww'),*source(43,'xxxx'),*source(44,'xxxx')]
        # r4.x still holds the selected spotlight ID at the final store.
        encode=[0x05000056,*dest(44,4),*source(4,'xxxx')]
        encode += [0x07000000,*dest(44,4),*source(44,'zzzz'),0x00004001,0x3f800000]
        encode += [0x08000000,*dest(44,2),0x80100006,0x41,44,0x00004001,0x3f800000]
        # Match the original UAV/address operand and change only the store value.
        assert w[14948+5:14948+7]==source(1)
        w[14953:14955]=source(44)
        for at,words in sorted([(231,init),(10549,winner),(14948,encode)],reverse=True):
            w[at:at]=words
    return transform_shex(data,transform)
