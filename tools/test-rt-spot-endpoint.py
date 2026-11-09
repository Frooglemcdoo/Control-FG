"""Check endpoint bounds and consistent miss/hit semantics on synthetic segments."""
import base64,hashlib,json,math,random,sys
from pathlib import Path
from rt_spot_endpoint import BASE_SHA,CAP_FRACTION,RADIUS_SCALE,endpoint_guard,llvm_float
root=Path(__file__).resolve().parents[1]
m=json.loads((root/'assets/rt-spot-endpoint/manifest.json').read_text())
b=base64.b64decode((root/'assets/rt-spot-endpoint/rt_spot_endpoint.dxil.b64').read_text())
assert len(b)==m['bytes'] and hashlib.sha256(b).hexdigest()==m['sha256']
assert m['baseline_sha256']==BASE_SHA and m['maximum_segment_fraction']==CAP_FRACTION
assert m['radius_scale']==RADIUS_SCALE
assert b[:4]==b'DXBC'
def endpoint(length,radius):
    if math.isnan(radius):radius=0
    return length*(1-2**-20)-min(max(radius,0)*RADIUS_SCALE,length*CAP_FRACTION)
for length in (0,1e-6,.01,.1,1,10,100,50000):
    for radius in (-10,0,.001,.1,1,100,float('nan'),float('inf')):
        end=endpoint(length,radius)
        assert math.isfinite(end) and 0<=end<=length
        assert end>=length*(1-CAP_FRACTION-2**-20)-1e-10
        # Miss initialization equals TMax: never count a miss as a blocker.
        assert not end<end
        if length:
            assert .5*length<end, 'Mid-segment occluder lost'
            assert .01*length<end, 'Receiver-side contact lost'
# Concrete fixture case: 10-unit segment, 0.2 emitter radius.
end=endpoint(10,.2)
assert 9.99>end and 9.9<end
# R113 skipped a blocker at 9.9. R114 must preserve that blocker.
assert 9.9>10*(1-2**-20)-min(.2,1)
assert abs((10*(1-2**-20)-end)-.02)<1e-10
assert endpoint(10,0)==10*(1-2**-20)
if len(sys.argv)>1:
    source=Path(sys.argv[1]).read_text();patched,edits=endpoint_guard(source)
    for i in range(4):
        assert patched.count(f'%SpotSegment{i}Max')==4 # definition, miss, TraceRay, comparison
    restored=patched
    for before,after in edits:restored=restored.replace(after,before)
    assert restored==source, 'Unrelated shader instructions changed'
    for i in range(4):
        assert f'%SpotSegment{i}ScaledRadius = fmul fast float %SpotSegment{i}PositiveRadius, {llvm_float(RADIUS_SCALE)}' in patched
        assert f'%SpotSegment{i}Cap = fmul fast float %SpotSegment{i}Length, {llvm_float(CAP_FRACTION)}' in patched
        assert f'float %SpotSegment{i}ScaledRadius, float %SpotSegment{i}Cap)' in patched
print('PASS endpoint binary hash, segment cap, zero/invalid radius, contact/mid blockers, terminal exclusion and miss semantics')
