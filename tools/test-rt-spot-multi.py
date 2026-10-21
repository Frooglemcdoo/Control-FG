"""Validate configurable spotlight loop transformation and packaged sidecars."""
import base64
import hashlib
import json
import re
import sys
from pathlib import Path
from rt_spot_samples import multi_spot_samples

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root/'assets/rt-spot/manifest.json').read_text())
runtime = (root/'src/rt_spot_runtime.h').read_text()
for name, expected in manifest['outputs'].items():
    data = base64.b64decode((root/'assets/rt-spot'/f'{name}.b64').read_text())
    assert len(data) == expected['bytes']
    assert hashlib.sha256(data).hexdigest() == expected['sha256']
    assert expected['sha256'] in runtime, f'Runtime rejects packaged {name}'
assert manifest['native_multi_ray'] in runtime
# Exact R71 single-ray binary is the release contract, not a visual-fix claim.
assert manifest['outputs']['rt_spot_full.dxil']['sha256'] == 'f0127f6267359eccf61fda126ecc9ff733d0b9090f6e9d16a88f65840e7d3ab8'
if len(sys.argv) > 1:
    native = Path(sys.argv[1]).read_text()
    result = multi_spot_samples(native)
    raygen = 'define void @"\\01?spotRayGeneration'
    assert native.split(raygen)[0] == result.split(raygen)[0]
    # The only edit after the spotlight loop is its own output normalization.
    native_tail = native.split('; <label>:265 ',1)[1]
    result_tail = result.split('; <label>:265 ',1)[1].replace(
        '%.i0368 = fmul fast float 2.500000e-01, %vShadowResult.1.i0',
        '%.i0368 = fmul fast float %352, %vShadowResult.1.i0')
    assert native_tail == result_tail
    assert native.count('call void @dx.op.traceRay') == result.count('call void @dx.op.traceRay')
    assert '%258 = fcmp fast olt float %257, %SpotFullDistance' in result
    assert '%262 = and i1 %258, %261' in result  # Misses excluded from nearest hit.
    assert '%236 = fmul fast float 2.000000e+00, %177' in result
    assert 'i32 157, %dx.types.Handle %256, i32 0, i32 1' in result
    assert '%exitcond446 = icmp eq i32 %263, 4' in result
    assert '%231 = uitofp i32 %228 to float' in result
    assert '%211 = icmp sgt i32 4, 0' in result
    assert '%.i0368 = fmul fast float 2.500000e-01, %vShadowResult.1.i0' in result
    assert '%.i1369 = fmul fast float %352, %vShadowResult.3.i1' in result
    assert '%SpotSample0Max' not in result
    # Interpret the emitted sign selection: all quadrants exactly once.
    signs = [(-1 if i & 1 else 1, -1 if i & 2 else 1) for i in range(4)]
    assert signs == [(1,1),(-1,1),(1,-1),(-1,-1)]
    assert '%exitcond = icmp eq i32 %350, %165' in result
print('PASS packaged shader hashes and configurable-loop transform when capture supplied')
