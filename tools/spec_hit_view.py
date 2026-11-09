"""R184: evaluate secondary local-light BRDF toward the primary surface.

Native r2.xyz*r2.w is the primary view-space position; r15.xyz is the
secondary view-space position at this site. Preserve all native shading
apart from its V direction, with the accepted immutable-load scheduling.
"""
import base64
import hashlib
import json
import struct
from pathlib import Path
from dxbc_transform import transform_shex
from spec_parallel import SOURCE_SHA, instructions, inst, src, dst, imm

ROOT=Path(__file__).resolve().parents[1]

def op(code,*args):
    body=sum(args,[])
    return [(len(body)+1)<<24 | code,*body]

def neg(reg,components='xyzx'):
    # Extended source operand: modifier NEG (1), extension type MODIFIER (1).
    a=src(reg,components)
    return [a[0]|0x80000000,0x41,*a[1:]]

def f(value):
    return imm(struct.unpack('<I',struct.pack('<f',value))[0])

def view_block(a):
    return (op(50,dst(19,'xyz'),src(2,'xyzx'),src(2,'wwww'),neg(15)) +
            op(16,dst(12,'w'),src(19,'xyzx'),src(19,'xyzx')) +
            op(49,dst(18,'w'),f(1e-12),src(12,'w')) +
            op(55,dst(19,'xyz'),src(18,'w'),src(19,'xyzx'),neg(15)) +
            op(16,dst(12,'w'),src(19,'xyzx'),src(19,'xyzx')) +
            a[444] + op(56,dst(19,'xyz'),src(12,'wwww'),src(19,'xyzx')))

def optimize(data):
    if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:
        raise ValueError('Unsupported native SPEC shader')
    def patch(w):
        a=instructions(w)
        assert len(a)==984
        assert a[443]==op(16,dst(12,'w'),neg(15),neg(15))
        assert a[445]==op(56,dst(19,'xyz'),src(12,'wwww'),neg(15))
        out=[]
        for i,ins in enumerate(a):
            if i in (405,406,444,445):continue
            if i==943:out+=a[405]+a[406]
            out+=view_block(a) if i==443 else ins
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)

def fixture(data,corrected,degenerate=False):
    """Execute the emitted production vector block against a shifted-camera
    reference, including sample loops, clamp, partial groups and sentinels.
    Distances vary by sample/pixel; homogeneous primary position has w=2.
    """
    def patch(w):
        a=instructions(w);body=[]
        # Integer positions avoid unrelated rounding in the shift identity.
        body+=op(86,dst(2,'xyz'),src(1,'xyzx'))  # uint -> float
        body+=op(56,dst(2,'xyz'),src(2,'xyzx'),f(.5))
        body+=inst('mov',dst(2,'w'),f(2))
        # Hit is primary plus a nonzero offset; degenerate case is hit=primary.
        for c,offset in zip('xyz',(3,4,5)):
            body+=op(50,dst(15,c),src(2,c),src(2,'w'),f(0 if degenerate else offset))
        if corrected:
            body+=view_block(a)
        else:
            if not degenerate:
                for c,offset in zip('xyz',(3,4,5)):
                    body+=inst('mov',dst(15,c),f(offset))
            body+=sum(a[443:446],[])
        # Map signed V to positive output, so the native final max won't mask
        # wrong direction components. Include alpha's native duplication.
        body+=op(50,dst(29,'xyz'),src(19,'xyzx'),f(.5),f(.5))
        out=sum(a[:143],[])+body+sum(a[956:],[])
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)

def main():
    data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    result=optimize(data);build=ROOT/'build';build.mkdir(exist_ok=True)
    (build/'spec-hit-view.dxbc').write_bytes(result)
    (build/'spec_hit_view.generated.h').write_text('inline constexpr unsigned char kSpecHitViewShader[]={\n'+','.join(map(str,result))+'\n};\n')
    for degenerate in (False,True):
        for corrected in (False,True):
            (build/f'spec-hit-view-{int(degenerate)}-{int(corrected)}.dxbc').write_bytes(fixture(data,corrected,degenerate))
    report=dict(source_sha256=SOURCE_SHA,sha256=hashlib.sha256(result).hexdigest(),bytes=len(result),
                extra_instructions=4,extra_temporaries=0,extra_texture_reads=0,ray_count_changed=False,
                hardware_quality_verified=False,hardware_performance_verified=False)
    (build/'spec-hit-view-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
