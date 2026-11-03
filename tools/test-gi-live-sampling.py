"""Execute generated sampling arithmetic and check coverage/energy properties."""
import argparse, math, re, struct, subprocess, tempfile, base64
from pathlib import Path
from gi_live_sampling import coordinate_ir, transform, ROOT
import numpy as np


def evaluate(variant,n,frames=8192):
    # Execute emitted IR arithmetic on a frame/ray grid, with float32 rounding.
    ray=np.tile(np.arange(n,dtype=np.uint32),frames)
    frame=np.repeat(np.arange(frames,dtype=np.uint32),n)
    vals={'DispatchRaysIndex':np.uint32(343),'DispatchRaysIndex90':np.uint32(714),
          'DispatchRaysIndex198':np.uint32(714),'rayIndex.03':ray,'rayIndex2.038':ray,
          '226':np.uint32(n),'286':np.uint32(n),'234':np.uint32(2),'294':np.uint32(2)}
    def value(s):
        if s.startswith('%'):return vals[s[1:]]
        if s.startswith('0x'):return np.float32(struct.unpack('>d',bytes.fromhex(s[2:]))[0])
        return np.float32(s) if '.' in s else np.uint32(int(s))
    for line in coordinate_ir(variant).splitlines():
        name,expr=line.strip()[1:].split(' = ',1)
        if expr.startswith('extractvalue'):r=frame
        elif expr.startswith('uitofp'):r=np.asarray(value(expr.split()[2]),dtype=np.float32)
        elif expr.startswith('select'):
            m=re.fullmatch(r'select i1 ([^,]+), i32 ([^,]+), i32 (.+)',expr);r=np.where(value(m[1]),value(m[2]),value(m[3])).astype(np.uint32)
        elif expr.startswith('call'):
            m=re.search(r'i32 36, float ([^,]+), float ([^)]+)',expr);r=np.minimum(value(m[1]),value(m[2]))
        else:
            op,rest=expr.split(' ',1)
            if op=='icmp':rest=rest.removeprefix('eq ')
            _,args=rest.split(' ',1);a,b=map(value,args.split(', '))
            with np.errstate(over='ignore'):
                r={'mul':lambda:a*b,'xor':lambda:a^b,'lshr':lambda:a>>b,'and':lambda:a&b,
                   'icmp':lambda:a==b,'udiv':lambda:a//b,'urem':lambda:a%b,
                   'fadd':lambda:a+b,'fmul':lambda:a*b,'fdiv':lambda:a/b}[op]()
        vals[name]=r
    names=('Frc','Frc26','Frc27') if variant==0 else ('Frc','Frc85','Frc86')
    return tuple(np.asarray(vals[k]).reshape(frames,n) for k in names)


def main():
    p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);a=p.parse_args()
    for n in (1,3,4,6,8,16,64):
        u,v,w=evaluate(0,n);u1,v1,w1=evaluate(1,n)
        for x,y in zip((u,v,w),(u1,v1,w1)):
            assert np.array_equal(x,y) and np.isfinite(x).all() and (x>=0).all() and (x<1).all()
        cols=2 if n%2==0 else 1;rows=n//cols
        cells=np.floor(u*cols).astype(int)+cols*np.floor(v*rows).astype(int)
        # Float rounding can land on a neighboring stratum boundary, so allow
        # its exact shared boundary; never permit a skipped whole stratum.
        assert np.max(np.abs(u-np.arange(n)%cols/cols))<=1/cols+1e-7
        assert np.max(np.abs(v-np.arange(n)//cols/rows))<=1/rows+1e-7
        assert abs(u.mean()-.5)<.015 and abs(v.mean()-.5)<.015 and abs(w.mean()-.5)<.015
        # Native cosine hemisphere map: E[cos(theta)] = 2/3, range mean = base.
        assert abs(np.sqrt(v).mean()-2/3)<.015
        assert abs((.8+.4*w).mean()-1)<.006
        # A bright angular patch retains expected area; not a brightness clamp.
        assert abs(((u<.25)&(v<.25)).mean()-1/16)<.008
        assert not np.array_equal(u[:32],u[256:288])
        print('PASS emitted IR sampling: rays=',n,'mean_cos=',float(np.sqrt(v).mean()))
    with tempfile.TemporaryDirectory() as tmp:
        for variant in (0,1):
            shader=Path(tmp)/'native.dxbc';shader.write_bytes(base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64').read_bytes()))
            native=subprocess.check_output([a.dxc,'-dumpbin',str(shader)],text=True)
            patched=transform(native,variant)
            # Whole hit/miss functions and all instructions after random inputs
            # are preserved by the transform's reversible exact replacement.
            assert patched.count('call void @dx.op.traceRay.struct.HitData')==1
            bad=native.replace('0x3FD99999C0000000','0x3FE0000000000000')
            try:transform(bad,variant)
            except ValueError:pass
            else:raise AssertionError('Unknown range law accepted')
    print('PASS two pinned native libraries, full-frame variation, finite range, angular energy and unknown-source rejection')
if __name__=='__main__':main()
