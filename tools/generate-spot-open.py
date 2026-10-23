"""R112 visibility isolation: preserve selected light shading, bypass its RT weight.

Only the visibility operand of the final two additive MADs changes. All native
material, cookie, falloff, nonselected lighting and point-light code stays intact.
This is a diagnostic, not a proposed shadow fix.
"""
import base64
import hashlib
import struct
from pathlib import Path
from dxbc_transform import transform_shex
from rt_spot_contribution import R94_COMPOSITION_SHA
from rt_spot_history import dest, source


def force_selected_visible(data):
    if hashlib.sha256(data).hexdigest() != R94_COMPOSITION_SHA:
        raise ValueError('Unsupported spotlight composition')
    def patch(words):
        for offset, dst, color, swizzle, acc, accsw in (
            (5986,9,0,'xywx',9,'xyzx'), (6009,8,8,'xyzx',0,'xywx')):
            expected=[0x09000032,*dest(dst,15),*source(7,'zzzz'),*source(color,swizzle),*source(acc,accsw)]
            assert words[offset:offset+9]==expected
            # Scalar immediate has the same operand length as r7.zzzz.
            words[offset+3:offset+5]=[0x00004001,0x3f800000]
    return transform_shex(data,patch)


def main():
    root=Path(__file__).resolve().parents[1]
    native=base64.b64decode((root/'assets/rt-spot/rt_spot_light.dxbc.b64').read_text())
    patched=force_selected_visible(native)
    # Byte-exact instruction comparison: no point or material code may change.
    def words(data):
        for off in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32):
            if data[off:off+4]==b'SHEX':
                size=struct.unpack_from('<I',data,off+4)[0]
                return list(struct.unpack_from('<'+'I'*(size//4),data,off+8))
        raise AssertionError('Missing SHEX')
    a,b=words(native),words(patched)
    allowed={5989,5990,6012,6013}
    assert len(a)==len(b) and all(x==y for i,(x,y) in enumerate(zip(a,b)) if i not in allowed)
    out=root/'build';out.mkdir(exist_ok=True)
    (out/'rt_spot_open.dxbc').write_bytes(patched)
    (out/'rt_spot_open.h').write_text('#pragma once\nstatic const unsigned char kSpotOpenShader[]={\n'+','.join(f'0x{x:02x}' for x in patched)+'\n};\n')
    print('PASS: selected-light open diagnostic changes only two visibility operands; sha256='+hashlib.sha256(patched).hexdigest())

if __name__=='__main__':main()
