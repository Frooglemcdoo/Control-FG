"""Verify the exact composition change and layer-specific fade behavior."""
from pathlib import Path
import math
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
from rt_spot_native_blend import native_blend,visibility
base=root/'build/rt_spot_bent_normal_off.dxbc';output=root/'build/rt_spot_native_blend.dxbc'
assert output.read_bytes()==native_blend(base.read_bytes())
a=decode(base)['instructions'];b=decode(output)['instructions']
extra=[i for i in b if 1716<=i['dword']<1755]
assert [(i['name'],i['operands']) for i in extra]==[
 ('and',['r[22].x','r[4].wwww','l(770)']),
 ('ieq',['r[22].x','r[22].xxxx','l(256)']),
 ('add',['r[22].y','r[7].zzzz','l(-1)']),
 ('mad',['r[22].y','r[11].yyyy','r[22].yyyy','l(1)']),
 ('movc',['r[7].z','r[22].xxxx','r[22].yyyy','r[7].zzzz'])]
remaining=[i for i in b if i not in extra]
assert len(a)==len(remaining)
for old,new in zip(a,remaining):
 assert old['name']==new['name']
 if old['dword']==170:assert new['operands']==['raw:0x17']
 else:assert old['operands']==new['operands'],(old,new)
# Independent source provenance: actual flags/fade loads and their live lanes.
old={i['dword']:i for i in a}
assert old[770]['operands']==['r[11].xy','r[5].x','l(196)','t[13][13].xyxx']
assert old[790]['operands']==['r[4].w','r[5].x','l(304)','t[13][13].xxxx']
assert old[815]['operands']==['r[14].xy','r[4].wwww','l(2,768,0,0)']
assert old[1683]['operands']==['r[0].x','r[0].x','r[11].y','l(1)']
assert old[1692]['operands']==['r[0].x','-r[11].y','r[0].x']
for i in a:
 if 800<=i['dword']<1716 and i['operands']:
  dest=i['operands'][0]
  assert not (dest.startswith('r[4].') and 'w' in dest.split('.')[1])
  assert not (dest.startswith('r[11].') and 'y' in dest.split('.')[1])
# Every combination of native layer bits; unrelated flags never change scope.
for flags in range(1024):
 for fade in [0,.25,.31153208017349243,1]:
  for rt in [0,.125,.5,1]:
   expected=rt
   if not(flags&2) and flags&768==256:expected=(1-fade)+fade*rt
   result=visibility(flags,fade,rt)
   assert math.isclose(result,expected,abs_tol=1e-7)
   assert 0<=result<=1
assert math.isclose(visibility(0x101,.31153208017349243,0),.6884679198265076)
assert visibility(0x103,.31153208017349243,0)==0
assert visibility(0x201,.31153208017349243,0)==0
assert visibility(0x101,1,0)==0 and visibility(0x101,0,0)==1
try:native_blend(base.read_bytes()[:-1])
except ValueError:pass
else:raise AssertionError('Unsupported shader accepted')
print('PASS native PCF blend: exact five-instruction scope, live field provenance, all layer combinations, fade endpoints and captured factor')
