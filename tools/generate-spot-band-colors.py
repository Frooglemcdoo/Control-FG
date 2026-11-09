"""Map raw blocker bands to categorical colors before native material composition."""
import base64
import hashlib
from pathlib import Path
from dxbc_transform import transform_shex
from rt_spot_debug_views import emit, floats, OP
from rt_spot_history import dest, source

OP['ge'] = 29
root = Path(__file__).resolve().parents[1]
data = base64.b64decode((root/'assets/rt-spot/rt_spot_visibility.dxbc.b64').read_text())

def patch(words):
    anchor = emit('mov', dest(0,7), source(7,'zzzz'))
    matches = [i for i in range(len(words)) if words[i:i+len(anchor)] == anchor]
    assert len(matches) == 1
    code = emit('mov',dest(0,7),floats(0,0,1,0)) # earlier blocker: blue
    for threshold,color in ((.25,(1,.25,0,0)),(.625,(1,0,0,0)),(.875,(0,1,0,0))):
        code += emit('ge',dest(1,1),source(7,'zzzz'),floats(threshold,threshold,threshold,threshold))
        code += emit('movc',dest(0,7),source(1,'xxxx'),floats(*color),source(0))
    i=matches[0];words[i:i+len(anchor)]=code

result=transform_shex(data,patch)
out=root/'build';out.mkdir(exist_ok=True)
(out/'rt_spot_band_colors.dxbc').write_bytes(result)
(out/'rt_spot_band_colors.h').write_text('#pragma once\nstatic const unsigned char kSpotBandColorsShader[]={\n'+','.join(f'0x{x:02x}' for x in result)+'\n};\n')
print('PASS generated categorical blocker colors '+hashlib.sha256(result).hexdigest())
