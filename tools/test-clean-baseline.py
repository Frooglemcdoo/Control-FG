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

# R22 may observe the prepared list once on F9; its production replay must
# otherwise remain byte-for-byte R21 (admission, validation and substitutions).
replay = (root / 'src/rr_diffuse_replay.h').read_text()
replay = replay.replace('#include "rr_world_shader_audit.h"\n', '')
replay = replay.replace('        if(auditShaders)RRWorldExportShaderAudit(frame,c->replay);\n', '')
assert hashlib.sha256(replay.encode()).hexdigest() == 'b6b11939b0570b7f7dda60805f8fdcd919024ab8e4fcb4d55f567c76eba0d916'
print('PASS: replay rendering body matches R21 after removing the two diagnostic additions')
