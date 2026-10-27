"""Audit register provenance/liveness and interpret emitted direction tokens."""
import base64,math,random,re,struct
from pathlib import Path
from dxbc_transform import transform_shex
from spec_parallel import instructions
from spec_hit_view import optimize,view_block

root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
def extract(blob):
    a=[];transform_shex(blob,lambda w:a.extend(instructions(w)));return a
a=extract(data);b=extract(optimize(data))
lines=[x.strip() for x in (root/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines()
       if x.strip() and not x.strip().startswith('//')]
assert lines[70:75]==[
 'mul r2.xyzw, r0.yyyy, CB0[0][15].xyzw',
 'mad r2.xyzw, CB0[0][14].xyzw, r0.xxxx, r2.xyzw',
 'mad r2.xyzw, CB0[0][16].xyzw, r0.zzzz, r2.xyzw',
 'add r2.xyzw, r2.xyzw, CB0[0][17].xyzw','rcp r2.w, r2.w']
for line in lines[75:965]:
    assert not re.match(r'\w+ r2\.',line),line
assert lines[446]=='dp3_sat r12.w, r20.xyzx, r19.xyzx'
assert lines[447]=='add r18.w, -r7.w, l(1.000000)'
assert lines[428]=='add r15.xyz, r15.xyzx, CB0[0][5].xyzx'
# Undo the entire direction edit, then reverse the accepted two-load move.
assert b[441:448]==instructions([0,0,*view_block(a)])
b[441:448]=a[443:446]
assert b[:405]+b[941:943]+b[405:941]+b[943:]==a
for bad in (data[:-1],data[:-1]+bytes([data[-1]^1])):
    try:optimize(bad)
    except ValueError:pass
    else:raise AssertionError('Unknown source accepted')

def execute(regs):
    # Independent operand decoder for the actual generated SM5 tokens.
    for ins in instructions([0,0,*view_block(a)]):
        code=ins[0]&2047;dest=ins[1];r=ins[2];pos=3;args=[]
        while pos<len(ins):
            token=ins[pos];pos+=1;modifier=0
            if token>>31:
                modifier=(ins[pos]>>6)&255;pos+=1
            kind=(token>>12)&255
            if kind==4:
                val=struct.unpack('<f',struct.pack('<I',ins[pos]))[0];pos+=1;v=[val]*4
            else:
                assert kind==0
                values=regs[ins[pos]];pos+=1
                if token&12==8:v=[values[(token>>4)&3]]*4
                else:v=[values[(token>>(4+2*i))&3] for i in range(4)]
            if modifier:
                assert modifier==1;v=[-x for x in v]
            args.append(v)
        if code==50:out=[x*y+z for x,y,z in zip(*args)]
        elif code==16:out=[sum(x*y for x,y in zip(args[0][:3],args[1][:3]))]*4
        elif code==49:out=[float(x<y) for x,y in zip(*args)]
        elif code==55:out=[x if c else y for c,x,y in zip(*args)]
        elif code==68:out=[1/math.sqrt(x) for x in args[0]]
        elif code==56:out=[x*y for x,y in zip(*args)]
        else:raise AssertionError(code)
        regs.setdefault(r,[0.]*4)
        for i in range(4):
            if dest&(1<<(i+4)):regs[r][i]=out[i]
    return regs[19][:3]

rng=random.Random(184)
for i in range(10000):
    primary=[rng.uniform(-20,20) for _ in range(3)]
    hit=primary[:] if i%10==0 else [rng.uniform(-20,20) for _ in range(3)]
    w=rng.choice((.25,.5,1.,2.,4.))
    v=[x-y for x,y in zip(primary,hit)]
    if sum(x*x for x in v)<=1e-12:v=[-x for x in hit]
    n=math.sqrt(sum(x*x for x in v));expected=[x/n for x in v]
    actual=execute({2:[*(x/w for x in primary),w],15:[*hit,0]})
    assert max(abs(x-y) for x,y in zip(expected,actual))<1e-12
print('PASS primary/hit coordinate provenance, scratch liveness, exact reversible edit, 10000 decoded-token direction/fallback cases; GPU execution is a separate gate')
