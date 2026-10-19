"""Prevent world-renderer changes from entering the clean-baseline guide build."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'tools/fixtures/clean-baseline-world.json').read_text())
for name, expected in manifest['files'].items():
    data = (root / name).read_bytes().replace(b'\r\n', b'\n')
    assert hashlib.sha256(data).hexdigest() == expected, name
print(f"PASS: {len(manifest['files'])} production files match clean baseline {manifest['commit']}")

# The R23 extension must retain independent shader-validation budgets.
replay = (root / 'src/rr_diffuse_replay.h').read_text()
assert 'RRAlbedoShader::Session worldShaders;' in replay
assert 'c->replay.familyKinds[i]==1?worldShaders:shaders' in replay
assert 'worldAdmission.Allow(original.shader,c->replay.batches[i].shader,selectedShaders.PixelShaders()[result.selectedIndex])' in replay
print('PASS: world admission uses a separate validation budget and exact-pair gate')
