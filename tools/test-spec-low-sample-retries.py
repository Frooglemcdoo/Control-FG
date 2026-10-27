"""Contract and numerical checks for R179; no claim of in-game quality parity."""
import json
import math
import random
import re
import statistics
import struct
from spec_low_sample_retries import ROOT, transform
from spec_retry_reuse import transform as reuse

source = (ROOT/'research/native-integration-audit/reflection-000.ll').read_text()
patched = transform(source)
baseline = reuse(source)
entry = 'define void @"\\01?reflectionRayGeneration@@'
def outside(text):
    a=text.index(entry); b=text.index('\n}',a)+2
    return text[:a],text[b:]
assert outside(patched)==outside(source)
for token in ('@dx.op.traceRay.', '@dx.op.textureStore.', 'icmp ult i32 %294, 100',
              '%292 = fcmp', '%316 = fcmp', '%exitcond =', '%Frc63 =', '%Frc64 =', '%Frc65 ='):
    assert [l for l in source.splitlines() if token in l]==[l for l in patched.splitlines() if token in l],token
assert '%RetryCanReuse = and i1 %RetryCached, %NativeSpec' in patched
assert 'br i1 %LowSpec, label %LowRetry, label %NativeRetry' in patched
native=patched.split('NativeRetry:\n')[1].split('  br label %LowRetryMerge')[0]
native=native.replace('%NativeRetryU =','%Frc =').replace('%NativeRetryV =','%Frc47 =')
assert native in source
assert '[ %NativeRetryU, %NativeRetry ], [ %LowUUniform, %LowRetry ]' in patched
assert '[ %NativeRetryV, %NativeRetry ], [ %LowVUniform, %LowRetry ]' in patched
for count in range(1,65):
    low=(count==2 or count==4)
    assert (not low)==(count not in (2,4))
    # R176 caching is still used for every other sample count.
    for cached in (False,True): assert (cached and not low)==(cached and count not in (2,4))

# Execute emitted integer/float IR, rather than testing only a separate hash.
ops=patched.split('LowRetry:\n')[1].split('  br label')[0].strip().splitlines()
def f32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x): return struct.unpack('<I',struct.pack('<f',x))[0]
def emitted(index,pixel,attempt,noise):
    env={'%272':index&0xffffffff,'%257':pixel,'%294':attempt,'%.i0258':noise[0],'%.i1260':noise[1]}
    def value(x):
        if x in env:return env[x]
        if x.startswith('0x'):return struct.unpack('>d',bytes.fromhex(x[2:]))[0]
        return int(x)
    for line in ops:
        name,rhs=line.strip().split(' = ')
        if rhs.startswith('bitcast float '): v=bits(value(rhs.split()[2]))
        elif rhs.startswith('uitofp i32 '): v=f32(value(rhs.split()[2]))
        else:
            op,typ,operands=rhs.split(' ',2); aa,bb=map(value,operands.split(', '))
            if op=='xor':v=aa^bb
            elif op=='lshr':v=aa>>bb
            elif op=='mul':v=(aa*bb)&0xffffffff
            elif op=='fmul':v=f32(aa*bb)
            else:raise AssertionError(rhs)
        env[name]=v
    return env['%LowUUniform'],env['%LowVUniform']
def mix(x):
    x&=0xffffffff; x^=x>>16; x=x*0x7feb352d&0xffffffff
    x^=x>>15; x=x*0x846ca68b&0xffffffff; return x^(x>>16)
def uniform(index,pixel,attempt,noise):
    seed=((index*0x9e3779b9)^pixel^bits(noise[0])^((bits(noise[1])*668265261)&0xffffffff)^(attempt*0x85ebca6b))&0xffffffff
    return (mix(seed)>>8)/2**24,(mix(seed^0x68bc21eb)>>8)/2**24
rng=random.Random(179)
for _ in range(1000):
    args=(rng.randrange(2**32),rng.randrange(2**32),rng.randrange(1,101),(rng.random(),rng.random()))
    assert emitted(*args)==uniform(*args)
    assert all(0<=x<1 for x in emitted(*args))
# Different noise texels must break the common quantized world-cell seed.
assert emitted(1,0,1,(.2,.3))!=emitted(1,0,1,(.21,.3))
assert len({emitted(i,0,1,(.2,.3)) for i in range(4)})==4

A=struct.unpack('>d',bytes.fromhex('3FE827F540000000'))[0]
B=struct.unpack('>d',bytes.fromhex('3FE23C21C0000000'))[0]
def direction(uv,rough,ndv):
    u=uv[1]*.4; c=math.sqrt((1-u)/(1+u*(rough**4-1)))
    s=math.sqrt(max(0,1-c*c));phi=uv[0]*2*math.pi
    h=(s*math.cos(phi),s*math.sin(phi),c); v=(math.sqrt(1-ndv*ndv),0,ndv)
    d=2*sum(a*b for a,b in zip(h,v))
    return tuple(d*a-b for a,b in zip(h,v))
def sequence(index,noise):return ((noise[0]+index*A)%1,(noise[1]+index*B)%1)
def samples(count,frame,pixel,noise,rough,ndv,new):
    rays=[];rejects=0;retries=0;cached=None
    for sample in range(count):
        index=frame*count+sample;d=direction(sequence(index,noise),rough,ndv)
        if d[2]<.005:
            rejects+=1
            if cached is not None and not new:d=cached
            else:
                for attempt in range(1,101):
                    uv=uniform(index,pixel,attempt,noise) if new else sequence(frame+attempt*10,noise)
                    d=direction(uv,rough,ndv);retries+=1
                    if d[2]>=.005:break
                cached=d
        rays.append(d)
    return rays,rejects,retries
light=(-.8,.1,.59);length=math.sqrt(sum(x*x for x in light));light=tuple(x/length for x in light)
def radiance(d):return math.exp((sum(a*b for a,b in zip(d,light))-1)*100)
report=[]
for count in (2,4):
    for rough,ndv in ((.2,.1),(.5,.1),(.8,.1),(.8,.3),(1,.1)):
        # Synthetic smooth bright lobe, not a captured scene or RR reconstruction.
        rows={'native':[],'candidate':[]};rejected=0;costs=[0,0]
        for _ in range(8192):
            noise=(rng.random(),rng.random()); frame=rng.randrange(65536);pixel=rng.randrange(2**32)
            for mode,key in enumerate(rows):
                ds,rs,attempts=samples(count,frame,pixel,noise,rough,ndv,mode==1)
                rows[key].append(sum(map(radiance,ds))/count);costs[mode]+=attempts
                if mode==0:rejected+=rs
        reference=[]
        for _ in range(65536):
            d=direction((rng.random(),rng.random()),rough,ndv)
            if d[2]>=.005:reference.append(radiance(d))
        ref=statistics.mean(reference); ref_var=statistics.variance(reference)
        mean=statistics.mean(rows['candidate']);var=statistics.variance(rows['candidate'])
        # Broad statistical sanity, not equality to native's correlated estimator.
        error=abs(mean-ref);limit=6*math.sqrt(var/8192+ref_var/len(reference))+.0001
        assert error<limit,(rough,ndv,error,limit)
        if rough==.2:assert rows['native']==rows['candidate']
        report.append(dict(samples=count,roughness=rough,ndotv=ndv,rejected_fraction=rejected/(8192*count),
                           conditional_reference=ref,native_mean=statistics.mean(rows['native']),candidate_mean=mean,
                           native_variance=statistics.variance(rows['native']),candidate_variance=var,
                           native_retry_attempts_per_pixel=costs[0]/8192,candidate_retry_attempts_per_pixel=costs[1]/8192))
for before,after in [('icmp ult i32 %294, 100','icmp ult i32 %294, 99'),
                     ('%268 = icmp sgt i32 %139, 0','%268 = icmp sgt i32 %139, 1')]:
    try:transform(source.replace(before,after,1))
    except (AssertionError,ValueError):pass
    else:raise AssertionError('Unsupported source mutation accepted')
(ROOT/'build').mkdir(exist_ok=True)
(ROOT/'build/spec-low-sample-statistics.json').write_text(json.dumps(dict(
    scope='CPU directional model; no textures, RR, visibility or GPU timing; not game quality evidence',
    trials_per_case=8192,counts=[2,4],cases=report),indent=2)+'\n')
print('PASS: emitted hash execution, source scope, unchanged tracing/hit handlers, retry cap, sample diversity, conditional-distribution checks')
for case in report:print(json.dumps(case))
