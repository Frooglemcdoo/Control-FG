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
