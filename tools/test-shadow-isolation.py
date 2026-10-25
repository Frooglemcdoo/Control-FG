"""Verify the generated diagnostics remove only their documented input loads."""
import base64
import importlib.util
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
spec=importlib.util.spec_from_file_location('isolation',root/'tools/generate-shadow-isolation.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
module.generate()
for name,offset,register,mask,other_input in [
 ('contact_off',346,7,'zw',338),
 ('screen_occlusion_off',338,7,'xy',346),
 ('selector_occlusion_off',4436,9,'xy',717)]:
 d=decode(root/'build'/f'rt_spot_{name}.dxbc')
 instructions={i['dword']:i for i in d['instructions']}
 changed=instructions[offset]
 assert changed['name']=='mov' and changed['operands']==[f'r[{register}].{mask}','l(1)']
 assert instructions[other_input]['name']=='ld'
 # Only the intended register lanes are overwritten. Visibility, sunlight's
 # separate shadow channel, bent normals, and material inputs stay distinct.
 assert all(instructions[offset+i]['name']=='nop' for i in (5,6,7))
 assert any(b['name']=='g_tBentNormals' for b in d['bindings'])
print('PASS: independent decoded instruction/lane checks for all three shaders')
data=base64.b64decode((root/'assets/rt-spot/rt_spot_light.dxbc.b64').read_text())
try:
 module.neutralize_load(data[:-1]+bytes([data[-1]^1]),module.LIGHT_SHA,346,7,12,(5,'xyww'),[0x002074e6,15,15])
except ValueError: pass
else: raise AssertionError('Modified input accepted')
print('PASS: unsupported shader rejected')

# R138 independently decode all changed operands and ensure the neighboring
# normal blend / light-color multiplication still consumes the neutral value.
for name,edits,neighbors in [
 ('bent_normal_off',[(2551,16,'xyz',0),(5116,17,'xyz',0),(7602,12,'xyz',0),(10023,6,'xyz',0)],[(2559,'mad'),(5124,'mad'),(7610,'mad'),(10031,'mad')]),
 ('selector_bent_normal_off',[(4745,10,'xyz',0)],[(8076,'mad'),(9765,'mad'),(12485,'mad'),(14030,'mad')]),
 ('local_maps_off',[(6987,4,'w',1)],[(6996,'mul'),(346,'ld')]),
 ('selector_local_maps_off',[(7321,43,'x',1),(12098,18,'w',1)],[(10464,'mul'),(10471,'mul'),(12107,'mul'),(717,'ld')])]:
 d=decode(root/'build'/f'rt_spot_{name}.dxbc');instructions={i['dword']:i for i in d['instructions']}
 for offset,register,mask,value in edits:
  assert instructions[offset]['name']=='mov'
  assert instructions[offset]['operands']==[f'r[{register}].{mask}',f'l({value})']
 for offset,opcode in neighbors: assert instructions[offset]['name']==opcode
print('PASS: bent-normal blend and local-map consumption boundaries remain intact')

# The motion comparison must preserve every R139 correction and change only
# the scalar screen-occlusion load, not sunlight or actual shadow visibility.
for name,baseline,offset,register,address,resource in [
 ('corrected_no_ao','bent_normal_off',338,7,(5,'xyww'),[0x002078d6,2,2]),
 ('selector_corrected_no_ao','selector_bent_normal_off',4436,9,(9,'xyzw'),[0x002078d6,2,2])]:
 original=(root/'build'/f'rt_spot_{baseline}.dxbc').read_bytes()
 actual=(root/'build'/f'rt_spot_{name}.dxbc').read_bytes()
 import hashlib
 assert actual==module.neutralize_load(original,hashlib.sha256(original).hexdigest(),offset,register,3,address,resource)
 old={i['dword']:i for i in decode(root/'build'/f'rt_spot_{baseline}.dxbc')['instructions']}
 new={i['dword']:i for i in decode(root/'build'/f'rt_spot_{name}.dxbc')['instructions']}
 assert new[offset]['operands']==[f'r[{register}].xy','l(1)']
 for pos,instruction in old.items():
  if pos!=offset: assert new[pos]==instruction
print('PASS: combined motion variants preserve all R139 operands outside scalar AO')

# Only selected spotlight visibility may change against the accepted baseline.
old={i['dword']:i for i in decode(root/'build/rt_spot_bent_normal_off.dxbc')['instructions']}
new={i['dword']:i for i in decode(root/'build/rt_spot_corrected_open.dxbc')['instructions']}
assert old.keys()==new.keys()
for pos,instruction in old.items():
 if pos in (5986,6009):
  assert new[pos]['name']==instruction['name']=='mad'
  assert new[pos]['operands'][1]=='l(1)'
  assert new[pos]['operands'][:1]+new[pos]['operands'][2:]==instruction['operands'][:1]+instruction['operands'][2:]
 else: assert new[pos]==instruction
print('PASS: corrected open changes only two selected spotlight visibility operands')
