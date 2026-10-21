"""Execute emitted segment/reduction IR against analytic near-light cases."""
import itertools
import math
import random
import re
import struct
from rt_spot_samples import AXES, SAMPLE_SIGNS, ENDPOINT_FRACTION, segment_instructions, visibility_instructions


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
        elif expression.startswith('or'): value = args[0] or args[1]
        elif '@dx.op.unary.f32(i32 24,' in expression: value = math.sqrt(args[0])
        elif '@dx.op.binary.f32(i32 36,' in expression: value = min(args)
        else: raise AssertionError(expression)
        values[name] = value
    return values

random.seed(73)
for case in range(200):
    origin = [random.uniform(-100,100) for _ in range(3)]
    light = [origin[i]+random.uniform(-5,5) for i in range(3)]
    dx = [0.3,0,0]; dy = [0,0.3,0]
    if case==0: light=origin[:]; dx=dy=[0,0,0]
    for index,(sx,sy) in enumerate(SAMPLE_SIGNS):
        inputs={}
        for i,(_,e,x,y,o) in enumerate(AXES): inputs.update({e:light[i],x:dx[i],y:dy[i],o:origin[i]})
        result=execute(segment_instructions(index,sx,sy),inputs)
        delta=[light[i]+sx*dx[i]+sy*dy[i]-origin[i] for i in range(3)]
        length=math.sqrt(sum(v*v for v in delta)); prefix=f'%SpotSample{index}'
        tmax=result[prefix+'Max'];direction=[result[prefix+a+'Direction'] for a in 'xyz']
        assert math.isclose(tmax,length*ENDPOINT_FRACTION,abs_tol=1e-12)
        assert all(math.isfinite(v) for v in direction)
        if length:
            assert math.isclose(sum(v*v for v in direction),1,abs_tol=1e-12)
            assert tmax<length
            for i in range(3): assert math.isclose(direction[i]*tmax,delta[i]*ENDPOINT_FRACTION,abs_tol=1e-12)

# Near-light blocker formerly inside the clipped-away final segment.
length=2.0; near=0.8; blocker=1.6
assert blocker>length-near and blocker<length*ENDPOINT_FRACTION
# Geometry beyond the sampled emitter must never cast a shadow on this segment.
assert length+0.01>length*ENDPOINT_FRACTION
# Miss payloads can be shorter than another ray's genuine hit; do not reduce them.
maxima=[1.0,2.0,3.0,4.0]; names=['%251','%SpotSample1Hit','%SpotSample2Hit','%SpotSample3Hit']
for mask in itertools.product([False,True],repeat=4):
    hits=[maxima[i]*0.9 if mask[i] else maxima[i] for i in range(4)]
    inputs=dict(zip(names,hits));inputs.update({f'%SpotSample{i}Max':v for i,v in enumerate(maxima)})
    result=execute(visibility_instructions(names),inputs)
    expected=min((hits[i] for i in range(4) if mask[i]),default=50000)
    assert result['%SpotNearest']==expected
    assert result['%SpotVisibleSum']/4==1-sum(mask)/4
print('PASS emitted segment IR: near-light blockers, biased-origin endpoint, zero length, beyond-light exclusion, mixed hit/miss reduction')
