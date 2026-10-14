"""Check the actual R7 capture against the earlier native input ABI."""
import hashlib
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'research/native-integration-audit/tools'))
from decode_dxbc import decode

def layout(value):
    if isinstance(value, dict):
        return {k: layout(v) for k, v in value.items() if k != 'type_offset'}
    if isinstance(value, list):
        return [layout(v) for v in value]
    return value

profiles = re.findall(r'\{(\d+),"([0-9a-f]{64})","([^"]+)"\}',
                     (root / 'src/pt_primary_profiles.h').read_text())
decoded = []
for filename, (size, sha, _) in zip(('single-spot.dxbc', 'standard.dxbc'), profiles):
    p = root / 'validation/primary-variants' / filename
    data = p.read_bytes()
    assert len(data) == int(size) and hashlib.sha256(data).hexdigest() == sha
    decoded.append(decode(p))
assert len(decoded) == 2
old, active = decoded
assert layout(old['constant_buffers']) == layout(active['constant_buffers'])
assert [b for b in old['bindings'] if b['name'] != 'g_rwtSingleSpot'] == active['bindings']
extra = [b for b in old['bindings'] if b['name'] == 'g_rwtSingleSpot']
assert len(extra) == 1 and extra[0]['kind'] == 4 and extra[0]['slot'] == 2
for shader in decoded:
    outputs={b['name']:(b['kind'],b['slot'],b['return_type'],b['dimension']) for b in shader['bindings'] if b['kind']==4}
    assert outputs['g_rwtLightBufferDiffuse']==(4,0,5,4)
    assert outputs['g_rwtLightBufferSpecular']==(4,1,5,4)
    light_cb = next(c for c in shader['constant_buffers'] if c['name']=='deferredlight_constants')
    fields = {v['name']:v for v in light_cb['variables']}
    for name, offset in (('g_uPointLightTotalCount',256),('g_uSpotLightTotalCount',260),
                         ('g_fAmbientEnvSpecularIntensityOnOpaque',244)):
        assert fields[name]['offset']==offset and fields[name]['size']==4
        assert not fields[name]['flags']&2, 'These lanes need providers, not native CBV reads'
    assert fields['g_uSunLightTotalCount']['offset']==264 and fields['g_uSunLightTotalCount']['flags']&2
    used = {(v['offset'],v['size']) for v in light_cb['variables'] if v['flags']&2}
    assert used=={(160,8),(168,4),(192,12),(216,8),(224,4),(228,4),(264,4)}
    groups = [i['operands'] for i in shader['instructions'] if i['name'] == 'dcl_thread_group']
    assert groups == [['raw:0x8,0x8,0x1']]
print('PASS: both captured shaders match exact profiles; all consumed bindings, constant layouts and 8x8 groups agree.')

# Standard primary adds g_tFillSpecular once, in radiance units, after the
# point/spot/sun lighting. Pin the actual instructions behind R30's restore.
fill=next(b for b in active['bindings'] if b['name']=='g_tFillSpecular')
assert (fill['kind'],fill['slot'],fill['space'],fill['dimension'])==(2,20,0,4)
instructions=active['instructions']
pos=next(i for i,v in enumerate(instructions) if v['name']=='sample_l' and 't[20][20].xyzw' in v['operands'])
assert instructions[pos]['operands']==['r[3].xyz','r[0].zwzz','t[20][20].xyzw','s[1][6]','l(0)']
assert instructions[pos-1]['operands']==['r[0].zw','r[0].zzzw','cb[0][0][1].zzzw']
assert instructions[pos+1]['name']=='add' and instructions[pos+1]['operands']==['r[2].xyzw','r[2].xyzw','r[3].xyzx']
print('PASS: native specular fill uses t20, s6 space1, pixel-center inverse-output UV and one radiance addition.')

material=next(c for c in active['constant_buffers'] if c['name']=='g_sbMaterialDataPart1')
fields=material['variables'][0]['type']['members']
assert any(f['name']=='uScatterIndex_vTranslucencyDepthRange' and f['byte_offset']==4 for f in fields)
print('PASS: material translucency range occupies the second material word; unsupported scattering retains native shading.')

# Guard the production registration path, not only the profile lookup table.
runtime = (root / 'src/pt_glass_runtime.h').read_text()
registration = runtime.split('static void ObserveCompute(', 1)[1].split('primaryPipelines.Insert', 1)[0]
assert re.search(r'const auto\* profile=PTPrimaryProfiles::Find\([^;]+;\s*if\(profile\)\s*\{\s*if\($', registration), 'Every exact reviewed profile must reach primary registration'
print('PASS: production primary registration accepts both exact reviewed profiles without a variant-name exclusion.')
