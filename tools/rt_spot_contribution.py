"""Omit only the selected spotlight's two final additive contributions.

Keep accumulated nonselected diffuse/specular and the entire native point-light
branch. This is a live diagnostic to measure how much the chosen light can affect
the final image, not a proposed shadow rendering mode.
"""
import hashlib
from dxbc_transform import transform_shex
from rt_spot_debug_views import emit
from rt_spot_history import dest, source

R94_COMPOSITION_SHA = 'a844888cdd5478605574157b323cb3376369ae9517c4954bf38bee21b664ecd7'


def selected_light_off(data):
    if hashlib.sha256(data).hexdigest() != R94_COMPOSITION_SHA:
        raise ValueError('Unsupported R94 composition')
    def transform(words):
        for offset, register, accumulator, swizzle in (
            (5986, 9, 9, 'xyzx'), (6009, 8, 0, 'xywx')
        ):
            token = words[offset]
            assert token & 2047 == 50  # mad
            size = (token >> 24) & 127
            replacement = emit('mov', dest(register,15), source(accumulator,swizzle))
            assert len(replacement) <= size
            words[offset:offset+size] = replacement + [0x0100003a]*(size-len(replacement))
    return transform_shex(data,transform)
