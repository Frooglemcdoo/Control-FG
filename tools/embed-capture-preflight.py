"""Embed hash-verified R97 library and the exact live global root."""
import base64, hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
entries=[('assets/rt-spot/rt_spot_full.dxil.b64','26688736d38b5ea056ae6bb37dc1a738334d94c784003a67957f3677f789a246','NativeSpot'),
         ('assets/rt-lights/native_global.rootsig.b64','451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3','NativeRoot')]
header='#pragma once\n'
for path,sha,name in entries:
    data=base64.b64decode((root/path).read_text())
    assert hashlib.sha256(data).hexdigest()==sha,path
    header+=f'static const unsigned char k{name}[]={{'+','.join(f'0x{x:02x}' for x in data)+'};\n'
(root/'build').mkdir(exist_ok=True)
(root/'build/capture_preflight_assets.h').write_text(header)
print('PASS preflight assets match R97 library and live global root')
