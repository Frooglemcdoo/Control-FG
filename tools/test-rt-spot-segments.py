"""Execute generated segment instructions against analytic endpoint cases."""
import math, random, re, struct, itertools
from rt_spot_segments import AXES, ENDPOINT_FRACTION, segment_instructions

def execute(lines, inputs):
    values = dict(inputs)
    def read(token):
        if token.startswith('%'): return values[token]
        if token.startswith('0x'): return struct.unpack('>d', bytes.fromhex(token[2:]))[0]
        return float(token)
    for line in lines:
        name, expression = line.strip().split(' = ',1)
        operands = re.findall(r'%[\w.]+|0x[0-9A-F]+|(?<![\w.])[-+]?\d+\.\d+e[-+]\d+', expression)
        args = [read(v) for v in operands if not v.startswith('%dx.')]
        if expression.startswith('fadd'): value = args[0]+args[1]
        elif expression.startswith('fsub'): value = args[0]-args[1]
        elif expression.startswith('fmul'): value = args[0]*args[1]
        elif expression.startswith('fdiv'): value = args[0]/args[1]
        elif expression.startswith('fcmp fast ogt'): value = args[0]>args[1]
        elif expression.startswith('fcmp fast olt'): value = args[0]<args[1]
        elif expression.startswith('select'): value = args[1] if args[0] else args[2]
        elif expression.startswith('and'): value = args[0] and args[1]
        elif expression.startswith('or'): value = args[0] or args[1]
        elif '@dx.op.unary.f32(i32 24,' in expression: value = math.sqrt(args[0])
        elif '@dx.op.binary.f32(i32 35,' in expression: value = max(args)
        elif '@dx.op.binary.f32(i32 36,' in expression: value = min(args)
        else: raise AssertionError(expression)
        values[name] = value
    return values

random.seed(96)
for case in range(200):
    origin = [random.uniform(-100,100) for _ in range(3)]
    light = [origin[i]+random.uniform(-5,5) for i in range(3)]
    disk = [random.uniform(-0.3,0.3) for _ in range(2)]
    if case == 0: light=origin[:]; disk=[0,0]
    for index in range(4):
        # Orthogonal X/Y vectors consistent with the native basis register aliases.
        inputs = {'%.0.i0':1, '%.027.i0':0, '%.0.i2':0, '%.027.i1':1, '%.027.i2':0,
                  f'%SpotDisk{index}X':disk[0], f'%SpotDisk{index}Y':disk[1]}
        for j,(_,emitter,_,_,start) in enumerate(AXES): inputs.update({emitter:light[j],start:origin[j]})
        result=execute(segment_instructions(index),inputs)
        delta=[light[j]-origin[j]+(disk[j] if j<2 else 0) for j in range(3)]
        length=math.sqrt(sum(v*v for v in delta)); prefix=f'%SpotSegment{index}'
        tmax=result[prefix+'Max'];direction=[result[prefix+a+'Direction'] for a in 'xyz']
        assert math.isclose(tmax,length*ENDPOINT_FRACTION,abs_tol=1e-12)
        assert all(math.isfinite(v) for v in direction)
        if length:
            assert math.isclose(sum(v*v for v in direction),1,abs_tol=1e-12)
            assert tmax<length
            for j in range(3): assert math.isclose(direction[j]*tmax,delta[j]*ENDPOINT_FRACTION,abs_tol=1e-12)
# Blocker in the former cutoff region is now inside the segment; geometry
# beyond the light remains excluded. These are analytic cases, not game proof.
assert 2.0-0.8 < 1.6 < 2.0*ENDPOINT_FRACTION < 2.01
# Execute the emitted hit/miss and mean-blocker instructions with unequal extents.
from rt_spot_samples import four_spot_samples
from rt_spot_segments import emitter_bounded_segments
import sys
from pathlib import Path
if len(sys.argv)>1:
    source=Path(sys.argv[1]).read_text()
    result=emitter_bounded_segments(four_spot_samples(source))
    lines=result[result.index('  %SpotOccluded0 ='):result.index('  br label %.loopexit32',result.index('  %SpotOccluded0 ='))].splitlines()
    lines=[line for line in lines if ' = ' in line]
    for mask in itertools.product([False,True], repeat=4):
        maxima=[0.5,2.0,3.0,4.0]
        hits=[maxima[i]*0.9 if mask[i] else maxima[i] for i in range(4)]
        inputs={'%SpotFullDistance':0.1, '%251':hits[0]}
        inputs.update({f'%SpotSegment{i}Max':maxima[i] for i in range(4)})
        inputs.update({f'%SpotSample{i}Hit':hits[i] for i in range(1,4)})
        values=execute(lines,inputs)
        blockers=[hits[i] for i in range(4) if mask[i]]
        assert values['%253']==1-sum(mask)/4
        assert math.isclose(values['%.vHitTResult.037.i0'],sum(blockers)/len(blockers) if blockers else 50000)
print('PASS emitted endpoint arithmetic: finite zero-length ray, near-light blocker, beyond-light exclusion; optional captured aggregation')
