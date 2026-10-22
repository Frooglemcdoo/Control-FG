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
random.seed(70)
for _ in range(1000):
    radius=math.sqrt(random.random());angle=random.random()*math.tau
    x,y=radius*math.cos(angle),radius*math.sin(angle)
    samples=[(x,y),(-x,y),(x,-y),(-x,-y)]
    assert abs(sum(p[0] for p in samples))<1e-12
    assert abs(sum(p[1] for p in samples))<1e-12
    assert all(abs(a*a+b*b-radius*radius)<1e-12 for a,b in samples)
if len(sys.argv)>1:
    import importlib.util
    spec=importlib.util.spec_from_file_location('generator',Path(__file__).with_name('generate-rt-spot-pair.py'))
    generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
    source=Path(sys.argv[1]).read_text();result=generator.full_distance(source,True)
    call='call void @dx.op.traceRay.struct.ShadowHitData('
    assert result.count(call)==source.count(call)+3
    assert result.count('i32 157, %dx.types.Handle %250, i32 0, i32 1') == 4
    assert '%215 = fmul fast float 2.000000e+00, %174' in result
    # Shared native alpha/closest-hit/miss handlers remain byte-identical.
    raygen = '; Function Attrs: nounwind\ndefine void @"\\01?spotRayGeneration'
    assert result.split(raygen)[0] == source.split(raygen)[0]
    point='; <label>:257 '
    assert result.split(point,1)[1]==source.split(point,1)[1], 'Point branch or handlers changed'
    assert result.count('float %SpotFullDistance, %struct.ShadowHitData* nonnull %11)') == 4
    assert '%SpotSample0Max' not in result
    for i in range(1,4):assert f'%SpotSample{i}Hit' in result
    assert '%253 = fmul fast float %SpotVisibleSum, 2.500000e-01' in result
    assert 'select i1 %255, float %SpotMeanBlocker, float 5.000000e+04' in result
print('PASS symmetric disk coverage, same-frame visibility/blocker aggregation, captured IR when provided')
