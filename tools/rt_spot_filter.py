"""Keep the spotlight penumbra approximation inside its finite domain.

Full-distance blockers can approach or enter the emitter radius. Native contact
filtering assumes radius/distance < 1; otherwise sqrt(1-ratio) becomes invalid.
Limit only that spotlight ratio to 0.99, keeping the point branch untouched.
"""
import struct
from dxbc_transform import transform_shex
from rt_spot_history import instruction, dest, source

SPATIAL_SHAS = (
    '584d4085443970868fe903d10bcdacb18c4c82490d08b742b5219648da437ab3',
    'e1a78e01b31559354cd8d474c22d117c444dd008a0067881d44836a23b0a459b',
)
MAX_RADIUS_RATIO = 0.99

def bounded_penumbra(data):
    def transform(words):
        assert words[425]&2047 == 14 and words[432]&2047 == 0
        # min r2.w, r2.w, 0.99; insert immediately after spotlight div.
        limit = struct.unpack('<I', struct.pack('<f', MAX_RADIUS_RATIO))[0]
        words[432:432] = instruction('min', dest(2,8), source(2,'wwww'), [0x4001,limit])
    return transform_shex(data, transform)
