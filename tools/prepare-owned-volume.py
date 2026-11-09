"""Recover the audited original media integrator for exact PSO admission and testing."""
import base64
import hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'research/owned-volume-r345/native-media.dxbc.b64').read_text())
assert hashlib.sha256(data).hexdigest()=='85f9289633e1fa90a18f12cb268ce48fdb5527e5a6e620084e3d8de1644f2d60'
(root/'build').mkdir(exist_ok=True)
(root/'build/native-media.dxbc').write_bytes(data)
print('PASS native participating-media shader identity')
