"""Audit local-specular provenance and reversible isolation edits."""
import base64,random,re,struct
from spec_local_components import ROOT,component,blocks,tail
from spec_hit_view import optimize
from spec_parallel import instructions
from dxbc_transform import transform_shex

data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
def extract(data):
    out=[];transform_shex(data,lambda w:out.extend(instructions(w)));return out
a=extract(data);baseline=extract(optimize(data))
lines=[x.strip() for x in (ROOT/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines() if x.strip() and not x.strip().startswith('//')]
assert lines[942]=='mad r15.xyz, r21.xywx, r29.xyzx, r34.xyzx'
assert lines[953]=='mad r14.xyw, r14.xyxw, r16.xyxz, r15.xyxz'
assert lines[954]=='add r29.xyz, r19.xyzx, r14.xywx'
# Track all writes to the local-light accumulators. r32 is point specular;
# r34 starts from r32 and adds spotlight specular. Neither includes env/emission.
writes32=[x for x in lines[448:955] if re.match(r'\w+ r32\.[xyz]',x)]
writes34=[x for x in lines[448:955] if re.match(r'\w+ r34\.[xyz]',x)]
assert writes32==['mov r32.xyz, l(0,0,0,0)','mad r32.xyz, r21.zzzz, r34.xyzx, r32.xyzx']
# Earlier point-loop r34 stores point-light color. Later r34 accumulates specs.
start=lines.index('mov r34.xyz, r32.xyzx')
assert [x for x in lines[start:955] if re.match(r'\w+ r34\.',x)]==[
    'mov r34.xyz, r32.xyzx','mad r34.xyz, r20.wwww, r35.xzwx, r34.xyzx']
for local in (False,True):
    b=extract(component(data,local))
    for ins in blocks(local).values():
        if ins:assert b.count(ins)==1;b.remove(ins)
    assert b[41]==[0x02000068,44];b[41]=baseline[41]
    # Remaining two substitutions occur after the R184 four-instruction delta.
    for offset in (957,960):
        idx=offset+4
        assert b[idx]!=baseline[idx]
        modified=baseline[idx].copy()
        if offset==957:modified[4]=43
        else:modified[-1]=43
        assert b[idx]==modified;b[idx]=baseline[idx]
    assert b==baseline
    for bad in (data[:-1],data[:-1]+bytes([data[-1]^1])):
        try:component(bad,local)
        except ValueError:pass
        else:raise AssertionError('Unrecognized shader accepted')

# Decode and execute the actual emitted component operations with independent
# channel values, catching packed xyw / xyz mistakes and component leakage.
def execute(local,env,diffuse,albedo,local_spec,emission):
    regs={14:[diffuse[0],diffuse[1],-99,diffuse[2]],16:[*albedo,-99],
          21:[.25,.5,-99,.75],29:[*env,-99],34:[*local_spec,-99],19:[*emission,-99],43:[0]*4}
    for index in (942,953,954):
        for ins in instructions([0,0,*blocks(local)[index]]):
            code=ins[0]&2047;dest=ins[1];reg=ins[2];args=[]
            for pos in range(3,len(ins),2):
                token=ins[pos];v=regs[ins[pos+1]]
                args.append([v[(token>>(4+2*i))&3] for i in range(4)])
            if code==54:values=args[0]
            elif code==56:values=[x*y for x,y in zip(*args)]
            elif code==50:values=[x*y+z for x,y,z in zip(*args)]
            elif code==0:values=[x+y for x,y in zip(*args)]
            else:raise AssertionError(code)
            for i in range(4):
                if dest&(1<<(i+4)):regs[reg][i]=values[i]
    return regs[43][:3]
rng=random.Random(186)
for _ in range(10000):
    env,diffuse,albedo,ls,emission=[[rng.random()*10 for _ in range(3)] for _ in range(5)]
    local=execute(True,env,diffuse,albedo,ls,emission)
    other=execute(False,env,diffuse,albedo,ls,emission)
    assert local==ls
    expected=[d*a+e*f+m for d,a,e,f,m in zip(diffuse,albedo,env,[.25,.5,.75],emission)]
    assert max(abs(x-y) for x,y in zip(other,expected))<1e-12
    total=[l+o for l,o in zip(local,other)];lum=sum(total)*.3333
    factor=min(lum,2.5)/lum
    assert max(abs((l*factor+o*factor)-t*factor) for l,o,t in zip(local,other,total))<1e-12
print('PASS local specular accumulator provenance; R184 recovery; 10000 decoded-token cases; original total cap basis; unknown source rejection')
