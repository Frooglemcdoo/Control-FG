"""Check shipped temporal isolation instructions and preserve point history."""
import base64
import random
import struct
from pathlib import Path
from dxbc_checksum import checksum

root = Path(__file__).resolve().parents[1]
data = base64.b64decode((root/'assets/rt-spot/rt_spot_history.dxbc.b64').read_text())
assert data[4:20] == checksum(data)
count = struct.unpack_from('<I',data,28)[0]
offsets = struct.unpack_from('<'+'I'*count,data,32)
offset = next(x for x in offsets if data[x:x+4] == b'SHEX')
size = struct.unpack_from('<I',data,offset+4)[0]
words = struct.unpack_from('<'+'I'*(size//4),data,offset+8)
patch = words[316:321]
assert patch == (0x05000036,0x001000d2,2,0x00100aa6,1)
assert words[321:329] == (0x08000000,0x001000f2,2,0x80100ae6,0x41,1,0x00100e46,2)
assert words[329]&2047 == 50  # Native multiply-add still blends point history.

# Interpret the actual MOV mask/swizzle before the native delta and blend.
mask = (patch[1] >> 4) & 15
swizzle = [(patch[3] >> (4+2*i)) & 3 for i in range(4)]
rng = random.Random(74)
for _ in range(1000):
    spot, point, old_spot, old_point, weight = [rng.random() for _ in range(5)]
    current = [old_spot,old_point,spot,point]
    history = [old_spot,old_point,old_spot,old_spot]
    for lane in range(4):
        if mask & (1 << lane): history[lane] = current[swizzle[lane]]
    targets = [spot,point,spot,spot]
    result = [c + weight*(h-c) for c,h in zip(targets,history)]
    assert result[0] == result[2] == result[3] == spot
    assert result[1] == point + weight*(old_point-point)
print('PASS shipped temporal isolation: spotlight current-only, point history preserved')
