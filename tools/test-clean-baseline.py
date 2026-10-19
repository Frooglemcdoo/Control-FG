"""Prevent world-renderer changes from entering the clean-baseline guide build."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'tools/fixtures/clean-baseline-world.json').read_text())
for name, expected in manifest['files'].items():
    data = (root / name).read_bytes().replace(b'\r\n', b'\n')
    if name in ('src/rr_native_frame.h','src/rr_specular_noisy.h'):
        # Reviewed R24 exceptions are checked by an exact reverse patch. The
        # original 132-file baseline manifest remains unchanged.
        changes = json.loads((root / 'tools/fixtures/r24-baseline-exceptions.json').read_text())[name]
        text = data.decode()
        for before, after in changes:
            assert after in text, (name, after)
            text = text.replace(after, before, 1)
        data = text.encode()
    assert hashlib.sha256(data).hexdigest() == expected, name
print(f"PASS: {len(manifest['files'])} production files preserve clean baseline outside reviewed R24 substitutions from {manifest['commit']}")

# The R23 extension must retain independent shader-validation budgets.
replay = (root / 'src/rr_diffuse_replay.h').read_text()
assert 'RRAlbedoShader::Session worldShaders;' in replay
assert 'c->replay.familyKinds[i]==1?worldShaders:shaders' in replay
assert 'worldAdmission.Allow(original.shader,c->replay.batches[i].shader,selectedShaders.PixelShaders()[result.selectedIndex])' in replay
print('PASS: world admission uses a separate validation budget and exact-pair gate')
