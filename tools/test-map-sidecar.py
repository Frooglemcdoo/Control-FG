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
    private=(operands and operands[0].startswith(('r[44]','r[45]'))) or (i['name']=='store_uav_typed' and operands[0].startswith('type30[3][3]')) or (i['name']=='dcl_unordered_access_view_typed' and operands==['raw:0x31ee46,0x3,0x3,0x3,0x5555,0x0'])
    (extra if private else remaining).append(i)
assert len(extra)==13
assert len(a)==len(remaining)
for old,new in zip(a,remaining):
    assert old['name']==new['name']
    if old['name']=='dcl_temps':assert new['operands']==['raw:0x2e']
    else:assert old['operands']==new['operands'],(old,new)
assert len([i for i in b if i['name']=='store_uav_typed' and i['operands'][0].startswith('type30[3][3]')])==2
try:map_visibility_sidecar(b'unknown')
except ValueError:pass
else:raise AssertionError('Unknown selector accepted')
print('PASS sidecar: exact original instruction stream, all five native stores retained; only private r44/r45/u3 added')

center=[i for i in b if i['operands'] and i['operands'][0]=='r[45].w']
assert [(i['name'],i['operands']) for i in center]==[
 ('mov',['r[45].w','l(-1,-1,-1,-1)']),
 ('sample_l',['r[45].w','r[40].xyxx','t[12][12].yzwx','s[1][6]','l(0)']),
 ('ge',['r[45].w','r[45].w','r[36].z']),
 ('and',['r[45].w','r[45].w','l(1)'])]
assert any(i['name']=='movc' and i['operands']==['r[44].w','r[14].wwww','r[45].wwww','r[44].wwww'] for i in b)
print('PASS PCF center: exact atlas/sampler/depth comparison, unsupported sentinel, same winner predicate')

# Execute the emitted sample/compare/winner block with synthetic atlas inputs.
from dxbc_transform import transform_shex
from rt_spot_history import dest,source

def fixture(w):
 def op(code,*args):
  body=[v for a in args for v in a];return [((len(body)+1)<<24)|code,*body]
 def emitted(i):
  at=i['dword'];return w[at:at+((w[at]>>24)&127)]
 code=w[:238]  # exact native declarations, including t12/s6 and private temps
 code+=w[246:250]  # native utof r0.xy, dispatchThreadID.xy
 code+=op(0,dest(40,3),source(0,'xyxx'),[0x4001,0x3f000000])
 code+=op(56,dest(40,3),source(40,'xyxx'),[0x4001,0x3e000000])
 code+=op(54,dest(36,4),[0x4001,0x3f000000])  # receiver comparison depth
 code+=emitted(center[0])  # unsupported sentinel, exactly as shipped
 code+=op(49,dest(14,1),source(0,'xxxx'),[0x4001,0x40e00000])
 code+=[0x0304001f,*source(14,'xxxx')]  # if_nz r14.x; x==7 has no PCF branch
 for i in center[1:]:code+=emitted(i)
 code+=[0x01000015]  # endif
 code+=op(54,dest(44,8),[0x4001,0x3e800000])  # previous candidate sentinel .25
 code+=op(49,dest(14,8),source(0,'yyyy'),[0x4001,0x40800000])  # winner only y<4
 save=next(i for i in b if i['name']=='movc' and i['operands'][0]=='r[44].w')
 code+=emitted(save)
 code+=op(54,dest(1,7),source(44,'wwww'))
 code+=op(54,dest(1,8),[0x4001,0x3f800000])
 stores=[i for i in b if i['name']=='store_uav_typed']
 diffuse=next(i for i in reversed(stores) if i['operands'][0].startswith('type30[0][0]'))
 specular=next(i for i in reversed(stores) if i['operands'][0].startswith('type30[1][1]'))
 code+=emitted(diffuse)
 # Replace only fixture output value with r1, after setting it to zero.
 code+=op(54,dest(1,15),[0x4001,0])
 store=emitted(specular);store[-2:]=source(1);code+=store+[0x0100003e]
 w[:]=code
(root/'build/rt_spot_map_center_test.dxbc').write_bytes(transform_shex(actual.read_bytes(),fixture))
