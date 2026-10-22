"""Verify the exact additive terms omitted by the selected-light diagnostic."""
import base64,struct,hashlib
from pathlib import Path
from rt_spot_contribution import selected_light_off
from rt_spot_debug_views import emit
from rt_spot_history import dest,source
root=Path(__file__).resolve().parents[1]/'assets/rt-spot'
native=base64.b64decode((root/'rt_spot_light.dxbc.b64').read_text())
actual=base64.b64decode((root/'rt_spot_selected_off.dxbc.b64').read_text())
assert selected_light_off(native)==actual

def words(data):
    offsets=struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32)
    offset=next(o for o in offsets if data[o:o+4]==b'SHEX')
    size=struct.unpack_from('<I',data,offset+4)[0]
    return list(struct.unpack_from('<'+'I'*(size//4),data,offset+8))
a,b=words(native),words(actual)
allowed=set()
for offset,dst,acc,swizzle in [(5986,9,9,'xyzx'),(6009,8,0,'xywx')]:
    size=(a[offset]>>24)&127
    expected=emit('mov',dest(dst,15),source(acc,swizzle))
    assert b[offset:offset+size]==expected+[0x0100003a]*(size-len(expected))
    allowed.update(range(offset,offset+size))
assert all(x==y for i,(x,y) in enumerate(zip(a,b)) if i not in allowed)
assert a[6025:]==b[6025:], 'Point-light branch changed'
assert len(native)==len(actual)
print('PASS selected-off shader: only selected diffuse/specular additions omitted; all other instructions and point branch unchanged')
