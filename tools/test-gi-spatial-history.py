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
    bounded = generate(source, bounded=True)
    assert bounded[4:20] == checksum(bounded)
    bc = chunks(bounded)
    assert all(bc[tag] == a[tag] for tag in a if tag != b'SHEX')
    with tempfile.TemporaryDirectory() as directory:
        p = Path(directory) / 'bounded.dxbc'; p.write_bytes(bounded); bj = decode(p)
    actual = bj['instructions']; pos = 0
    for instruction in audit['instructions']:
        if instruction['dword'] == 2121:
            inserted = actual[pos:pos+5]
            assert [i['name'] for i in inserted] == ['mad','max','mad','max','min']
            assert inserted[0]['operands'][:3] == ['r[13].xyz','r[1].xyzw','l(0.75)']
            assert inserted[2]['operands'][:3] == ['r[14].xyz','r[1].xyzw','l(1.25)']
            assert inserted[3]['operands'] == ['r[3].xyz','r[3].xyzw','r[13].xyzw']
            assert inserted[4]['operands'] == ['r[3].xyz','r[3].xyzw','r[14].xyzw']
            pos += 5
        expected = {k:v for k,v in instruction.items() if k != 'dword'}
        if instruction['dword'] == 90:
            expected['operands'] = ['raw:0xf']
        assert {k:v for k,v in actual[pos].items() if k != 'dword'} == expected
        pos += 1
    assert pos == len(actual)
    assert bj['bindings'] == audit['bindings']
    steady = generate(source, steady=True)
    assert steady[4:20] == checksum(steady)
    sc = chunks(steady)
    assert all(sc[tag] == a[tag] for tag in a if tag != b'SHEX')
    with tempfile.TemporaryDirectory() as directory:
        p = Path(directory) / 'steady.dxbc'; p.write_bytes(steady); sj = decode(p)
    assert len(sj['instructions']) == len(audit['instructions'])
    for old_instruction, new_instruction in zip(audit['instructions'], sj['instructions']):
        expected = {k:v for k,v in old_instruction.items() if k != 'dword'}
        if old_instruction['dword'] == 1807:
            expected = {'name':'mov', 'operands':['r[0].z','l(1)']}
        if old_instruction['dword'] == 1956:
            expected['operands'] = ['r[0].w','r[0].w','l(0)']
        assert {k:v for k,v in new_instruction.items() if k != 'dword'} == expected
    assert sj['bindings'] == audit['bindings']
    try:
        generate(source, bounded=True, steady=True)
    except ValueError:
        pass
    else:
        raise AssertionError('steady test accidentally permits final history')
    try:
        generate(source[:-1])
    except ValueError:
        pass
    else:
        raise AssertionError('unknown bytecode accepted')
print('PASS R259 both spatial variants: only final history coefficient changes; spatial math, descriptors, control flow and output ABI unchanged')
print('PASS R260 both bounded variants: five RGB-bound instructions; all original instructions preserved except scratch register count; no new samples or bindings')
print('PASS R261 both steady variants: fixed variance multiplier and zero final history only; geometry/material/hit weights, stencil, bindings and ABI preserved')
