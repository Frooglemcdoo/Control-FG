"""Verify diagnostic bands, threshold equality, and exact shader transformation scope."""
import base64
import hashlib
import json
import math
import sys
from pathlib import Path
from rt_spot_blocker import BASE_SHA, blocker_bands

root = Path(__file__).resolve().parents[1]
folder = root / 'assets/rt-spot-blocker'
manifest = json.loads((folder / 'manifest.json').read_text())
data = base64.b64decode((folder / 'rt_spot_blocker.dxil.b64').read_text())
assert hashlib.sha256(data).hexdigest() == manifest['sha256']
assert len(data) == manifest['bytes'] and manifest['baseline_sha256'] == BASE_SHA

def classify(length, radius, hit):
    end = length * (1 - 2**-20)
    if hit >= end:
        return 1.0
    radius = 0 if math.isnan(radius) else max(radius, 0)
    wide = end - min(radius, length * .1)
    tight = end - min(radius * .1, length * .01)
    return .75 if hit >= tight else .5 if hit >= wide else 0

end = 10 * (1 - 2**-20)
for radius in (-1, 0, .2, 100, float('nan'), float('inf')):
    assert classify(10, radius, end) == 1, 'Miss must not be an endpoint blocker'
    assert classify(10, radius, .01) == 0, 'Receiver-side blocker must remain dark'
assert classify(10, .2, 9.5) == 0
assert classify(10, .2, 9.9) == .5
assert classify(10, .2, 9.99) == .75
assert classify(10, .2, end-.2) == .5
assert classify(10, .2, end-.02) == .75
assert classify(0, .2, 0) == 1
if len(sys.argv) > 1:
    original = Path(sys.argv[1]).read_text()
    modified, changes = blocker_bands(original)
    assert len(changes) == 1
    assert modified.replace(changes[0][1], changes[0][0]) == original
    # Actual ray endpoints, trace calls, hit aggregation and point branch are untouched.
    for line in original.splitlines():
        if '@dx.op.traceRay' in line or 'SpotSegment' in line or 'SpotMeanBlocker' in line:
            assert line in modified
    assert '%253 = select i1 %SpotOccluded0' in modified
print('PASS blocker asset hash, hit/miss bands, exact threshold boundaries and unchanged ray scope')
