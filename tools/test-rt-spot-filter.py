"""Validate shipped filter instructions and the penumbra approximation domain."""
import base64
import math
import struct
from pathlib import Path
from dxbc_checksum import checksum
from rt_spot_filter import MAX_RADIUS_RATIO

root = Path(__file__).resolve().parents[1]
for index in range(2):
    data = base64.b64decode((root/f'assets/rt-spot/rt_spot_spatial{index}.dxbc.b64').read_text())
    assert data[4:20] == checksum(data)
    count = struct.unpack_from('<I', data, 28)[0]
    offsets = struct.unpack_from('<'+'I'*count, data, 32)
    offset = next(x for x in offsets if data[x:x+4] == b'SHEX')
    size = struct.unpack_from('<I',data,offset+4)[0]
    words = struct.unpack_from('<'+'I'*(size//4),data,offset+8)
    # Decode the actual added instruction, not a separate copy of its limit.
    assert words[432:438] == (0x07000033,0x00100082,2,0x00100ff6,2,0x4001)
    limit = struct.unpack('<f',struct.pack('<I',words[438]))[0]
    assert abs(limit-MAX_RADIUS_RATIO) < 1e-7
    assert words[439]&2047 == 0  # Original sqrt-input calculation follows.
    for ratio in [0,0.001,0.5,0.98,0.99,1,1.01,10,10000]:
        x = min(ratio,limit)
        angle = math.pi/2 - (((-0.0187292993*x+0.0742610022)*x-0.212114394)*x+1.57072878)*math.sqrt(1-x)
        width = math.tan(angle)
        assert math.isfinite(width) and width >= 0
        if ratio < 0.99: assert x == ratio
print('PASS shipped spotlight filter bounds: unchanged normal range, finite emitter-intersection range')
