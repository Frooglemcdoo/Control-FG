"""Independent segment/plane cases, transform preservation and signed asset checks."""
import base64
import hashlib
import json
import math
import random
import sys
from pathlib import Path
from rt_spot_authored_clip import BASE_SHA, ENDPOINT_SCALE, authored_clip, clip_distance
root=Path(__file__).resolve().parents[1]
folder=root/'assets/rt-spot-authored-clip'
m=json.loads((folder/'manifest.json').read_text())
data=base64.b64decode((folder/'rt_spot_authored_clip.dxil.b64').read_text())
assert hashlib.sha256(data).hexdigest()==m['sha256'] and len(data)==m['bytes']
assert m['baseline_sha256']==BASE_SHA and m['extra_rays']==0
# Light emits along +Z; the authored near plane is Z=1.
cone=(0,0,0);axis=(0,0,1);emitter=(0,0,.7)
for receiver in ((0,0,5),(4,0,5),(40,3,5)):
    length=math.dist(receiver,emitter)
    end=clip_distance(receiver,emitter,cone,axis,1)
    hit=[o+(e-o)*(end/length) for o,e in zip(receiver,emitter)]
    assert abs(hit[2]-1)<5e-6
    assert end>0 and end<length
    # Receiver-side and mid-segment contacts before the plane remain reachable.
    assert .001<end and .5*end<end
# Receiver inside/on the boundary, noncrossing ray and invalid metadata retain full length.
for receiver,light,normal,near in [((0,0,.8),emitter,axis,1),((0,0,1),emitter,axis,1),
    ((0,0,5),(0,0,2),axis,1),((0,0,5),emitter,(0,0,0),1),
    ((0,0,5),emitter,axis,-1),((0,0,5),emitter,axis,float('nan')),
    ((0,0,5),emitter,(float('inf'),0,1),1)]:
    assert clip_distance(receiver,light,cone,normal,near)==math.dist(receiver,light)*ENDPOINT_SCALE
# Arbitrary oriented planes; construct endpoints on known sides independently.
rng=random.Random(134)
for _ in range(1000):
    n=[rng.uniform(-1,1) for _ in range(3)];norm=math.sqrt(sum(x*x for x in n));n=[x/norm for x in n]
    c=[rng.uniform(-100,100) for _ in range(3)];near=rng.uniform(.01,10)
    tangent=[-n[1],n[0],0];start=rng.uniform(.01,20);behind=rng.uniform(.001,near)
    origin=[c[i]+n[i]*(near+start)+tangent[i]*3 for i in range(3)]
    target=[c[i]+n[i]*(near-behind) for i in range(3)]
    expected=math.dist(origin,target)*start/(start+behind)*ENDPOINT_SCALE
    assert abs(clip_distance(origin,target,c,n,near)-expected)<1e-10
if len(sys.argv)>1:
    source=Path(sys.argv[1]).read_text();patched,edits=authored_clip(source)
    for i in range(4):assert patched.count(f'%SpotSegment{i}Max')==4
    for before,after in edits:patched=patched.replace(after,before)
    assert patched==source, 'Receiver, direction, handlers, point shadows or filters changed'
print('PASS authored plane: oblique/rotated segments, close contacts, invalid/noncrossing fallback, signed shader and exact source reversal')
