"""Embed the compiled diagnostic library; gameplay shaders are not replaced."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=(root/'build/rt_spot_hit_capture.dxil').read_bytes()
assert data[:4]==b'DXBC'
(root/'build/rt_spot_hit_capture.h').write_text('#pragma once\nstatic const unsigned char kSpotHitCaptureShader[]={\n'+','.join(str(b) for b in data)+'\n};\n')
print('Embedded diagnostic hit library:',len(data),'bytes')
