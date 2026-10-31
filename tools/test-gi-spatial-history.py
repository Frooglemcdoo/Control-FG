import base64
import struct
import sys
import tempfile
from pathlib import Path
from gi_spatial_history import generate
from dxbc_checksum import checksum

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'research/native-integration-audit/tools'))
from decode_dxbc import decode

def chunks(data):
    return {data[o:o+4]: data[o+8:o+8+struct.unpack_from('<I', data, o+4)[0]]
            for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I', data, 28)[0], data, 32)}

for variant in range(2):
    source = base64.b64decode((root / f'research/gi-temporal-r208/diffuse-spatial-{variant}.dxbc.b64').read_bytes())
    result = generate(source)
    assert result[4:20] == checksum(result)
    a, b = chunks(source), chunks(result)
    assert a.keys() == b.keys()
    for tag in a:
        if tag != b'SHEX':
            assert a[tag] == b[tag]
    old = struct.unpack('<'+'I'*(len(a[b'SHEX'])//4), a[b'SHEX'])
    new = struct.unpack('<'+'I'*(len(b[b'SHEX'])//4), b[b'SHEX'])
    assert [(i, x, y) for i, (x, y) in enumerate(zip(old, new)) if x != y] == [(1962, 0x3f333333, 0)]
    with tempfile.TemporaryDirectory() as directory:
        p = Path(directory) / 'shader.dxbc'; p.write_bytes(source); audit = decode(p)
    by_offset = {i['dword']: i for i in audit['instructions']}
    assert by_offset[1905]['name'] == 'ieq'  # pass+1 == passCount
    assert by_offset[1914]['name'] == 'if'
    assert by_offset[2108]['operands'][2] == 't[6][6].xyzw'
    assert by_offset[2174]['name'] == 'store_uav_typed'
    try:
        generate(source[:-1])
    except ValueError:
        pass
    else:
        raise AssertionError('unknown bytecode accepted')
print('PASS R259 both spatial variants: only final history coefficient changes; spatial math, descriptors, control flow and output ABI unchanged')
