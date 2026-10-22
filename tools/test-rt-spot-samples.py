"""Check same-frame visibility and blocker aggregation plus real captured IR."""
import itertools, math, random, sys
from pathlib import Path
for hits in itertools.product([0.1,1.0,10.0],repeat=4):
    maximum=10.0
    visible=sum(h>=maximum for h in hits)/4
    nearest=min(hits)
    blockers=[h for h in hits if h<maximum]
    output=sum(blockers)/len(blockers) if blockers else 50000.0
    assert 0<=visible<=1
    assert (output==50000.0)==(visible==1)
    if blockers: assert min(blockers)-1e-12<=output<=max(blockers)+1e-12
    if len(set(blockers))==1: assert math.isclose(output,blockers[0],rel_tol=1e-12)
# Equal-area annuli avoid the old four-rays-at-one-radius failure: an
# annular blocker covering 25% of the emitter gets one hit, not 0 or 4 hits.
random.seed(92)
radial_sum = [0.0]*4
angular_x = angular_y = 0.0
count = 20000
for _ in range(count):
    area, turn = random.random(), random.random()
    samples=[]
    strata=[]
    for i in range(4):
        q=(area+i/4)%1
        angle=math.tau*((turn+i/4)%1)
        x,y=math.sqrt(q)*math.cos(angle),math.sqrt(q)*math.sin(angle)
        samples.append((x,y))
        strata.append(int(q*4))
        radial_sum[i]+=q
        angular_x+=x; angular_y+=y
        assert math.isfinite(x+y) and x*x+y*y<=1+1e-12
    assert sorted(strata)==[0,1,2,3]
    assert sum(x*x+y*y<0.25 for x,y in samples)==1
assert all(abs(total/count-0.5)<0.01 for total in radial_sum)
assert abs(angular_x/(count*4))<0.01 and abs(angular_y/(count*4))<0.01
if len(sys.argv)>1:
    import importlib.util
    spec=importlib.util.spec_from_file_location('generator',Path(__file__).with_name('generate-rt-spot-pair.py'))
    generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
    source=Path(sys.argv[1]).read_text();result=generator.full_distance(source,True)
    call='call void @dx.op.traceRay.struct.ShadowHitData('
    assert result.count(call)==source.count(call)+3
    assert result.count('i32 157, %dx.types.Handle %250, i32 0, i32 1') == 4
    assert '%SpotDisk0ScaledRadius = fmul fast float %SpotDisk0Radius, %174' in result
    for i in range(4):
        assert f'%SpotDisk{i}Area = call float @dx.op.unary.f32(i32 22,' in result
    # Shared native alpha/closest-hit/miss handlers remain byte-identical.
    raygen = '; Function Attrs: nounwind\ndefine void @"\\01?spotRayGeneration'
    assert result.split(raygen)[0] == source.split(raygen)[0]
    point='; <label>:257 '
    assert result.split(point,1)[1]==source.split(point,1)[1], 'Point branch or handlers changed'
    assert result.count('float %SpotFullDistance, %struct.ShadowHitData* nonnull %11)') == 0
    for i in range(4): assert f'float %SpotSegment{i}Max, %struct.ShadowHitData* nonnull %11)' in result
    assert '%SpotSample0Max' not in result
    for i in range(1,4):assert f'%SpotSample{i}Hit' in result
    assert '%253 = fmul fast float %SpotVisibleSum, 2.500000e-01' in result
    assert 'select i1 %255, float %SpotMeanBlocker, float 5.000000e+04' in result
print('PASS stratified equal-area disk coverage, same-frame visibility/blocker aggregation, captured IR when provided')
