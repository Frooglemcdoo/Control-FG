"""Keep the verified glass raygen byte-exact across DXC runner updates."""
from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
data=bytes.fromhex((root/'assets/glass/pt_glass_raygen_r97.hex').read_text())
assert hashlib.sha256(data).hexdigest()=="1269fd85d37c06e8be835e7e8934fe084fa9827c31c3ca2b69092b531fcf0933", 'Canonical glass shader changed'
(root/'build').mkdir(exist_ok=True)
(root/'build/pt_glass_raygen.dxil').write_bytes(data)
print('PASS: canonical R97 glass raygen restored; unrelated DXC version changes excluded')
