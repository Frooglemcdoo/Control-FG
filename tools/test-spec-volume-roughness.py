"""R189 provenance, liveness, reversible shader edits and decoded-token math."""
import base64,random,struct,re
from spec_volume_roughness import ROOT,extract,optimize,blend_block
from spec_hit_view import optimize as baseline
from spec_parallel import instructions

data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
a=extract(data);b=extract(optimize(data));base=extract(baseline(data))
lines=[x.strip() for x in (ROOT/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines() if x.strip() and not x.strip().startswith('//')]
assert lines[367]=='if_nz r7.w' and lines[377]=='else'
assert lines[375:377]==['mul r27.xyz, r4.xxxx, r27.xyzx','mul r29.xyz, r4.xxxx, r29.xyzx']
assert lines[153]=='ieq r6.w, r5.w, l(0x0000ffff)'
assert lines[404]=='if_z r6.w'
# Smoothness comes from the material's authored range and texture channel.
assert 'mad r22.y, r7.w, r8.w, r22.x' in lines[158:190]
# r25.xyz no longer needed after normalized sums; overwritten before next use.
for line in lines[404:]:
    if re.search(r'\br25\.[xyz]',line):
        assert re.match(r'\w+ r25\.xyz,',line),line;break
# r7.w is overwritten before use in hit shading (miss skips that region).
assert lines[408].startswith('imad r7.w,')
block=instructions([0,0,*blend_block()]);pos=b.index(a[376])+1
assert b[pos:pos+5]==block
assert b[:pos]+b[pos+5:]==base
for bad in (data[:-1],data[:-1]+bytes([data[-1]^1])):
    try:optimize(bad)
    except ValueError:pass
    else:raise AssertionError('Unknown source accepted')

def execute(spec,diff,smoothness,miss):
    regs={7:[0]*4,6:[0,0,0,int(miss)],22:[0,smoothness,0,0],25:[0]*4,27:[*diff,0],29:[*spec,0]}
    for ins in block:
        code=ins[0]&2047;dest=ins[1];r=ins[2];pos=3;args=[]
        while pos<len(ins):
            token=ins[pos];pos+=1;modifier=0
            if token>>31:modifier=(ins[pos]>>6)&255;pos+=1
            kind=(token>>12)&255
            if kind==4:
                val=struct.unpack('<f',struct.pack('<I',ins[pos]))[0];pos+=1;v=[val]*4
            else:
                assert kind==0
                values=regs[ins[pos]];pos+=1
                v=[values[(token>>4)&3]]*4 if token&12==8 else [values[(token>>(4+2*i))&3] for i in range(4)]
            if modifier:assert modifier==1;v=[-x for x in v]
            args.append(v)
        if code==0:out=[x+y for x,y in zip(*args)]
        elif code==56:out=[x*y for x,y in zip(*args)]
        elif code==55:out=[x if c else y for c,x,y in zip(*args)]
        elif code==50:out=[x*y+z for x,y,z in zip(*args)]
        else:raise AssertionError(code)
        if ins[0]&0x2000:out=[min(1,max(0,x)) for x in out]
        for i in range(4):
            if dest&(1<<(i+4)):regs[r][i]=out[i]
    return regs[29][:3]

rng=random.Random(189)
for i in range(10000):
    spec=[rng.random()*100 for _ in range(3)];diff=[rng.random()*100 for _ in range(3)]
    if i%11==0:diff=spec[:]
    smooth=rng.choice([0,.5,1,-1,2]) if i%3==0 else rng.random();miss=i%7==0
    t=0 if miss else min(1,max(0,1-smooth))**2
    result=execute(spec,diff,smooth,miss)
    expected=[(1-t)*s+t*d for s,d in zip(spec,diff)]
    assert max(abs(x-y) for x,y in zip(result,expected))<1e-12
    assert all(min(s,d)-1e-12<=x<=max(s,d)+1e-12 for x,s,d in zip(result,spec,diff))
    if miss or smooth>=1:assert result==spec
print('PASS exact R184 recovery, source/liveness audit, 10000 decoded blend cases, convex bounds, flat-field/smooth/miss invariance and unknown-source rejection')
