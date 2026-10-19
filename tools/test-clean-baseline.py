"""Prevent world-renderer changes from entering the clean-baseline guide build."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'tools/fixtures/clean-baseline-world.json').read_text())
live_exceptions = json.loads((root/'tools/fixtures/r31-baseline-exceptions.json').read_text())
exceptions = json.loads((root/'tools/fixtures/r28-baseline-exceptions.json').read_text())
for name, expected in manifest['files'].items():
    data = (root / name).read_bytes().replace(b'\r\n', b'\n')
    if name in live_exceptions:
        text=data.decode()
        for before,after in reversed(live_exceptions[name]):
            assert text.count(after)==1, (name, 'R31 live UI insertion changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in exceptions:
        text=data.decode()
        for before,after in exceptions[name]:
            assert text.count(after)==1, (name, 'R28 capture insertion changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    assert hashlib.sha256(data).hexdigest() == expected, name
print(f"PASS: {len(manifest['files'])} baseline files verified, allowing exact R28 capture and R31 live UI/reset changes, against {manifest['commit']}")
