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
    groups = [i['operands'] for i in shader['instructions'] if i['name'] == 'dcl_thread_group']
    assert groups == [['raw:0x8,0x8,0x1']]
print('PASS: both captured shaders match exact profiles; all consumed bindings, constant layouts and 8x8 groups agree.')
