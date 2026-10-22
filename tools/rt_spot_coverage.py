"""Keep selected-light map coverage without multiplying two shadow estimates.

Input is the hash-locked native spotlight composition shader. r0.x holds
native map visibility at DWORD 1716; r7.z holds filtered RT visibility.
Combine once before the native zero gate, then neutralize its final RT factor.
Point lighting and every native material/attenuation calculation are preserved.
"""
from dxbc_transform import transform_shex
from rt_spot_history import instruction, dest, source

def preserve_coverage(data):
    def transform(words):
        assert words[1707] & 2047 == 55, 'Native map selection changed'
        assert words[1716] & 2047 == 57, 'Native map zero gate changed'
        assert words[5986] & 2047 == 50 and words[6009] & 2047 == 50, 'RT composition changed'
        words[1716:1716] = (
            instruction('min', dest(0,1), source(0,'xxxx'), source(7,'zzzz')) +
            instruction('mov', dest(7,4), [0x4001,0x3f800000]))
    return transform_shex(data, transform)
