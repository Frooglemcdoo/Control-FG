"""Spotlight-only filter isolation; point lanes retain native operations."""
from dxbc_transform import transform_shex
from rt_spot_history import instruction, dest, source

def raw_history(data):
    def transform(words):
        assert words[316:324] == [0x08000000,0x001000f2,2,0x80100ae6,0x41,1,0x00100e46,2]
        words[316:316] = instruction('mov',dest(2,13),source(1,'zzzz'))
    return transform_shex(data,transform)

def raw_spatial(data):
    def transform(words):
        assert words[1807]&2047 == 14 and words[1814]&2047 == 164
        # r4.x retains the center spotlight sample throughout the native loop.
        # Replace only spotlight output x after normalization; point y is intact.
        words[1814:1814] = instruction('mov',dest(1,1),source(4,'xxxx'))
    return transform_shex(data,transform)
