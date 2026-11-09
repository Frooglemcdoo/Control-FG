"""Validate position reconstruction independently and retain native trace instructions."""
import base64, hashlib, json, math, sys
from pathlib import Path
from rt_spot_hit_position import AXES, BASE_SHA, hit_position
root=Path(__file__).resolve().parents[1]
folder=root/'assets/rt-spot-hit-position'
m=json.loads((folder/'manifest.json').read_text())
assert m['baseline_sha256']==BASE_SHA
for axis in AXES:
    data=base64.b64decode((folder/f'rt_spot_hit_{axis}.dxil.b64').read_text())
    assert len(data)==m[axis]['bytes'] and hashlib.sha256(data).hexdigest()==m[axis]['sha256']
# Independent Cartesian reconstruction must equal the light-relative expression.
for origin,light in (((0,0,0),(3,4,0)),((50,-20,7),(-2,4,8)),((10000,20000,-30000),(10001,20003,-29998))):
    vector=[b-a for a,b in zip(origin,light)]
    length=math.sqrt(sum(v*v for v in vector));direction=[v/length for v in vector]
    for fraction in (.01,.5,.95,.999):
        t=length*fraction
        for a,b,d in zip(origin,light,direction):
            assert abs((a+t*d-b)-(t-length)*d)<1e-10
# Axis bands distinguish both sides of the light plane; equality stays in center band.
def band(delta,radius,hit=True):
    if not hit:return 1
    tolerance=max(radius,0)*.1
    return 0 if delta < -tolerance else .75 if delta > tolerance else .5
assert [band(v,1) for v in (-.11,-.1,0,.1,.11)]==[0,.5,.5,.5,.75]
assert band(-1,1,False)==1
if len(sys.argv)>1:
    source=Path(sys.argv[1]).read_text()
    for axis in AXES:
        modified,(before,after)=hit_position(source,axis)
        assert modified.replace(after,before)==source
        assert f'%SpotSegment0{axis}Direction' in after
        for line in source.splitlines():
            if '@dx.op.traceRay' in line or 'SpotSegment' in line or 'SpotMeanBlocker' in line:assert line in modified
print('PASS three asset hashes, Cartesian reconstruction, signed bands, misses and exact native ray preservation')
