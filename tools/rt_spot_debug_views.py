"""Direct spotlight signal views using the native composition resource layout.

Replace composition, not ray generation: t15.x is the spotlight visibility and
uint t14.x is its selected light ID (65535 means absent). The normal material,
light-color and map calculations are not executed. Later game postprocessing
still applies, so these views must be examined with RR/FG disabled.
"""
import hashlib
import struct
from dxbc_transform import transform_shex
from rt_spot_history import dest, source, immediate

NATIVE_LIGHT_SHA = '6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621'
OP = {'mov':54, 'movc':55, 'utof':86, 'mul':56, 'frc':26}

def emit(name, *operands):
    body = [word for operand in operands for word in operand]
    return [((len(body)+1)<<24)|OP[name], *body]

def floats(*values):
    return immediate([struct.unpack('<I', struct.pack('<f', x))[0] for x in values])

def debug_view(data, light_ids=False):
    if hashlib.sha256(data).hexdigest() != NATIVE_LIGHT_SHA:
        raise ValueError('Unsupported spotlight composition shader')
    def transform(w):
        assert w[170]&2047 == 104 and w[171] == 22
        assert w[303]&2047 == 54 and w[346]&2047 == 45
        assert w[354]&2047 == 45 and w[362]&2047 == 39
        assert w[10859]&2047 == 164 and w[10882]&2047 == 164
        # Native declarations, dispatch-pixel coordinates, visibility, light ID
        # and validity comparison. No native lighting/material accumulation.
        code = w[:176] + w[303:315] + w[346:372]
        if light_ids:
            code += emit('utof', dest(0,7), source(5,'xxxx'))
            code += emit('mul', dest(0,7), source(0), floats(.61803399,.41421356,.73205081,0))
            code += emit('frc', dest(0,7), source(0))
        else:
            code += emit('mov', dest(0,7), source(7,'zzzz'))
        code += emit('movc', dest(0,7), source(5,'zzzz'), source(0), floats(1,0,1,0))
        code += emit('mov', dest(0,8), floats(1,1,1,1))
        code += w[10859:10866]  # diffuse UAV = diagnostic RGB
        code += emit('mov', dest(0,15), floats(0,0,0,0))
        code += w[10882:10889]  # specular UAV = zero
        code += [0x0100003e]  # ret
        w[:] = code
    return transform_shex(data, transform)
