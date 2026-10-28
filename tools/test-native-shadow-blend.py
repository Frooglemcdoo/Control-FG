"""Verify the exact composition change and layer-specific fade behavior."""
from pathlib import Path
import math
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
from rt_spot_native_blend import native_blend,visibility
base=root/'build/rt_spot_bent_normal_off.dxbc';output=root/'build/rt_spot_native_blend_legacy.dxbc'
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

# R200 must differ from R199 only by saving the final native map result and
# applying one min after the existing fade. No point-light/material/resource edits.
from rt_spot_native_blend import native_map_guard, guarded_visibility
from dxbc_transform import transform_shex
from rt_spot_history import dest, source
import base64, hashlib, struct
protected=root/'build/rt_spot_native_blend.dxbc'
assert protected.read_bytes()==native_map_guard(base.read_bytes())
c=decode(protected)['instructions']
new=[i for i in c if i['dword'] in (1707,1764)]
assert [(i['name'],i['operands']) for i in new]==[
 ('movc',['r[22].z','r[1].w','r[6].w','r[0].x']),
 ('min',['r[7].z','r[7].zzzz','r[22].zzzz'])]
assert [(i['name'],i['operands']) for i in c if i not in new]==[(i['name'],i['operands']) for i in b]
# Restore the overwritten native movc and prove it against the archived native
# composition SHA, independently of the new destination register.
original=base64.b64decode((root/'assets/rt-spot/rt_spot_light.dxbc.b64').read_text())
def restore(w):w[1707:1716]=[0x09000037,0x00100012,0,0x0010003a,1,0x0010003a,6,0x0010000a,0]
assert hashlib.sha256(transform_shex(original,restore)).hexdigest()=='6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621'
for flags in range(1024):
 for fade in (0,.25,.31153208017349243,1):
  for rt in (0,.125,.5,1):
   for native_map in (0,.125,.5,1):
    prior=visibility(flags,fade,rt); actual=guarded_visibility(flags,fade,rt,native_map)
    assert actual<=prior and actual<=native_map and actual>=0
    if native_map==1:assert actual==prior
    if native_map==0:assert actual==0
    if native_map==prior:assert actual==prior # do not square a penumbra
assert guarded_visibility(0x101,.25,1,.75)==.75 # no second fade
assert guarded_visibility(0x101,1,.5,.5)==.5 # no map*RT double attenuation

# Numerical GPU fixture: extract the exact emitted save/fade/min instructions
# from the shipping shader. Synthetic inputs replace the preceding atlas work.
# Keep native resource declarations, dispatch coordinates, loads and UAV stores.
def fixture(w):
 def op(code,*args):
  body=[v for a in args for v in a];return [((len(body)+1)<<24)|code,*body]
 code=w[:176]+w[303:315]+w[346:362]
 code+=op(54,dest(4,8),source(5,'xxxx')) # flags
 code+=op(54,dest(11,2),source(5,'yyyy')) # float fade bits
 code+=op(54,dest(0,1),source(7,'wwww')) # native map result
 code+=op(56,dest(6,8),source(7,'wwww'),[0x4001,0x3f000000]) # alternate layer result
 code+=op(32,dest(1,8),source(4,'wwww'),[0x4001,514]) # native selection predicate
 code+=w[1707:1771] # save, bypass, unchanged fade, min
 code+=op(54,dest(0,1),source(7,'zzzz'))
 code+=op(54,dest(0,2),source(22,'zzzz'))
 code+=op(54,dest(0,4),source(7,'wwww')) # prove point visibility unchanged
 code+=op(54,dest(0,8),[0x4001,0x3f800000])
 # Locate the final native UAV stores by opcode and binding, not shifted offsets.
 stores=[i for i in c if i['name']=='store_uav_typed']
 diffuse=stores[-2]['dword'];specular=stores[-1]['dword']
 code+=w[diffuse:diffuse+7]
 code+=op(54,dest(0,15),[0x4001,0])
 code+=w[specular:specular+7]+[0x0100003e]
 w[:]=code
(root/'build/rt_spot_map_guard_test.dxbc').write_bytes(transform_shex(protected.read_bytes(),fixture))
print('PASS R200 exact two-instruction scope, native map provenance, all layer/fade combinations and no double shadowing; emitted GPU fixture')
