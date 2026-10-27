"""R187 provenance, component-token and diagnostic gain audits."""
import base64,random
from spec_indirect_components import ROOT,component,blocks,op,f,src,dst
from spec_hit_view import optimize
from spec_parallel import instructions
from dxbc_transform import transform_shex
data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
def extract(data):
    out=[];transform_shex(data,lambda w:out.extend(instructions(w)));return out
baseline=extract(optimize(data))
lines=[x.strip() for x in (ROOT/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines() if x.strip() and not x.strip().startswith('//')]
assert lines[941]=='mad r14.xyw, r22.xzxw, r27.xyxz, r17.xyxz'
assert lines[942]=='mad r15.xyz, r21.xywx, r29.xyzx, r34.xyzx'
assert lines[943]=='sample_l r16.xyz, r30.xyxx, T24[r6.w + 0].xyzw { nonuniform }, S0[5], l(0.000000)'
assert lines[944]=='mul r16.xyz, r23.yzwy, r16.xyzx'
assert lines[953]=='mad r14.xyw, r14.xyxw, r16.xyxz, r15.xyxz'
assert lines[954]=='add r29.xyz, r19.xyzx, r14.xywx'
gain=op(56,dst(14,'xyw'),src(14,'xyww'),f(8))
for environment in (False,True):
    b=extract(component(data,environment))
    for ins in blocks(environment).values():
        if ins:assert b.count(ins)==1;b.remove(ins)
    if environment:
        assert b[965]==gain # native 961 + four R184 instructions
        b.pop(965)
    assert b[41]==[0x02000068,44];b[41]=baseline[41]
    for offset in (957,960):
        idx=offset+4;expected=baseline[idx].copy()
        if offset==957:expected[4]=43
        else:expected[-1]=43
        assert b[idx]==expected;b[idx]=baseline[idx]
    assert b==baseline
    for bad in (data[:-1],data[:-1]+bytes([data[-1]^1])):
        try:component(bad,environment)
        except ValueError:pass
        else:raise AssertionError('Unknown shader accepted')

# Execute emitted component multiply tokens using independently randomized
# RGB channels and poisoned unused packed channels.
rng=random.Random(187)
for _ in range(10000):
    d,a,e,factor=[[rng.random()*10 for _ in range(3)] for _ in range(4)]
    regs={14:[d[0],d[1],-999,d[2]],16:[*a,-999],21:[factor[0],factor[1],-999,factor[2]],29:[*e,-999]}
    for environment in (False,True):
        ins=blocks(environment)[942 if environment else 953]
        assert ins[0]&2047==56
        args=[]
        for pos in (3,5):
            token=ins[pos];values=regs[ins[pos+1]]
            args.append([values[(token>>(4+2*i))&3] for i in range(3)])
        result=[x*y for x,y in zip(*args)]
        expected=[x*y for x,y in zip(e,factor)] if environment else [x*y for x,y in zip(d,a)]
        assert result==expected
# Actual gain token must multiply x,y,w in place, with an exactly encoded 8.
assert gain[1]&0xF0==0xB0 and gain[2]==14 and gain[-1]==0x41000000
print('PASS diffuse/environment provenance, exact R184 recovery, 10000 decoded RGB cases, post-cap 8x gain and unknown-source rejection')
