"""Prevent world-renderer changes from entering the clean-baseline guide build."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'tools/fixtures/clean-baseline-world.json').read_text())
for name, expected in manifest['files'].items():
    data = (root / name).read_bytes().replace(b'\r\n', b'\n')
    if name == 'src/rr_native_frame.h':
        text=data.decode()
        for before,after in json.loads((root/'tools/fixtures/r27-baseline-exceptions.json').read_text()):
            assert text.count(after) == 1, 'R27 hook insertion changed or duplicated'
            text=text.replace(after,before,1)
        data=text.encode()
    assert hashlib.sha256(data).hexdigest() == expected, name
print(f"PASS: {len(manifest['files'])} baseline files verified against {manifest['commit']} after reversing exact reviewed R27 hook insertions")
