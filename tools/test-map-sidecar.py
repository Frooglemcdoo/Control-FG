"""Verify instrumentation retains the exact native selector instruction stream."""
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
from rt_spot_selection import map_visibility_sidecar
original=root/'build/rt_spot_selector_bent_normal_off.dxbc'
actual=root/'build/rt_spot_selector_map_sidecar.dxbc'
assert actual.read_bytes()==map_visibility_sidecar(original.read_bytes())
a=decode(original)['instructions'];b=decode(actual)['instructions']
# Strip ONLY private-register arithmetic, new u3 declaration and u3 stores.
remaining=[];extra=[]
for i in b:
    operands=i['operands']
    private=(operands and operands[0].startswith('r[44]')) or (i['name']=='store_uav_typed' and operands[0].startswith('type30[3][3]')) or (i['name']=='dcl_unordered_access_view_typed' and operands==['raw:0x31ee46,0x3,0x3,0x3,0x5555,0x0'])
    (extra if private else remaining).append(i)
assert len(extra)==8
assert len(a)==len(remaining)
for old,new in zip(a,remaining):
    assert old['name']==new['name']
    if old['name']=='dcl_temps':assert new['operands']==['raw:0x2d']
    else:assert old['operands']==new['operands'],(old,new)
assert len([i for i in b if i['name']=='store_uav_typed' and i['operands'][0].startswith('type30[3][3]')])==2
try:map_visibility_sidecar(b'unknown')
except ValueError:pass
else:raise AssertionError('Unknown selector accepted')
print('PASS sidecar: exact original instruction stream, all five native stores retained; only private r44/u3 added')
