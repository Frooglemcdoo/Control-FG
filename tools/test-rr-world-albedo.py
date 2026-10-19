"""Verify R23 hash admission and the captured native diffuse calculation."""
from pathlib import Path
import hashlib
import importlib.util
import tempfile

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('dxbc', root / 'research/native-integration-audit/tools/decode_dxbc.py')
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)
runtime = (root / 'src/rr_world_albedo_runtime.h').read_text()
decoded = {}
for kind, size in [('original', 15072), ('albedo', 2908)]:
    data = bytes.fromhex((root / f'tools/fixtures/rr-world-{kind}.hex').read_text())
    assert len(data) == size
    assert hashlib.sha256(data).hexdigest() in runtime
    changed = bytearray(data)
    changed[100] ^= 1
    assert hashlib.sha256(changed).hexdigest() not in runtime
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / 'pixel.dxbc'
        path.write_bytes(data)
        decoded[kind] = decoder.decode(path)

def instruction(kind, offset, name, operands):
    item = next(x for x in decoded[kind]['instructions'] if x['dword'] == offset)
    assert item['name'] == name and item['operands'] == operands, item

# Both variants use material Part3 tint at byte 52, texture materialIndex*35
# (base slot zero), UV register 4 and sampler 19. The original then multiplies
# the sampled diffuse-light input by this reflectance, preserving specular.
instruction('original', 424, 'ld_structured', ['r[2].xyzw', 'cb[3][3][0].x', 'l(52)', 't[6][6].xyzw'])
instruction('original', 436, 'imul', ['null', 'r[0].w', 'cb[3][3][0].x', 'l(35)'])
instruction('original', 446, 'sample', ['r[3].xyzw', 'v[4].xyxx', 't[7][r[0].w].xyzw', 's[1][19]'])
instruction('original', 459, 'mul', ['r[2].xyzw', 'r[2].xyzw', 'r[3].xyzw'])
instruction('original', 466, 'mad', ['r[0].xyz', 'r[1].xyzx', 'r[2].xyzx', 'r[0].xyzx'])
instruction('albedo', 38, 'ld_structured', ['r[0].xyzw', 'cb[0][0][0].x', 'l(52)', 't[0][0].xyzw'])
instruction('albedo', 50, 'imul', ['null', 'r[1].x', 'cb[0][0][0].x', 'l(35)'])
instruction('albedo', 60, 'sample', ['r[1].xyzw', 'v[4].xyxx', 't[1][r[1].x].xyzw', 's[0][19]'])
instruction('albedo', 73, 'mul', ['o[0].xyzw', 'r[0].xyzw', 'r[1].xyzw'])
assert sum(x['name'].startswith('sample') for x in decoded['albedo']['instructions']) == 1
assert 'before.size!=15072' in runtime and 'albedo.bytes.size()!=2908' in runtime
print('PASS: captured native/base-albedo math agrees; exact SHA256 and size gates match both fixtures; mutated fixtures rejected')
