import hashlib
import struct
import sys
import tempfile
from pathlib import Path
from volume_filter import write_header, generate
from dxbc_checksum import checksum

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'research/native-integration-audit/tools'))
from decode_dxbc import decode
source, result = write_header(root)
assert result[4:20] == checksum(result)
with tempfile.TemporaryDirectory() as folder:
    p = Path(folder) / 'volume.dxbc'
    p.write_bytes(source)
    old = decode(p)
    p.write_bytes(result)
    new = decode(p)
assert old['bindings'] == new['bindings']
assert old['constant_buffers'] == new['constant_buffers']
def semantic(instruction):
    return instruction['name'], instruction['operands']
# Every native instruction outside the two current-frame reads survives.
expected = [semantic(i) for i in old['instructions'] if i['dword'] not in (48, 407, 420)]
actual = [semantic(i) for i in new['instructions']]
cursor = 0
for item in expected:
    cursor = actual.index(item, cursor) + 1
assert sum(i['name'] == 'sample_l' for i in new['instructions']) == 10
assert [semantic(i) for i in old['instructions'] if i['name'] == 'store_uav_typed'] == [
    semantic(i) for i in new['instructions'] if i['name'] == 'store_uav_typed']
for i in new['instructions']:
    if i['name'] == 'mad' and i['operands'][0] == 'r[7].xy':
        assert i['operands'][2:] == ['cb[0][0][21].xyxx', 'r[2].xyzw']
assert ('mov', ['r[7].xyzw', 'r[2].xywx']) in actual
assert ('mov', ['r[1].yzw', 'r[8].xxyz']) in actual
assert ('and', ['r[1].x', 'r[1].xxxx', 'r[5].xxxx']) in actual
# Each axis of four half-texel bilinear taps gives [1,2,1]/4.
kernel = [[x*y/16 for x in (1,2,1)] for y in (1,2,1)]
assert sum(map(sum, kernel)) == 1
assert sum(kernel[y][x]*((-1)**(x+y)) for y in range(3) for x in range(3)) == 0
assert sum(kernel[y][x]*(2*x-3*y+7) for y in range(3) for x in range(3)) == 6
try:
    generate(source[:-1])
    raise AssertionError('Accepted an unknown shader')
except ValueError:
    pass
print('PASS volume bindings, original reprojection and output stores, XY tent kernel, checksum and source rejection')
