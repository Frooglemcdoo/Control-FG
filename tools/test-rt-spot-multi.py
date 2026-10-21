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
if len(sys.argv) > 1:
    native = Path(sys.argv[1]).read_text()
    result = multi_spot_samples(native)
    raygen = 'define void @"\\01?spotRayGeneration'
    assert native.split(raygen)[0] == result.split(raygen)[0]
    assert native.split('; <label>:265 ',1)[1] == result.split('; <label>:265 ',1)[1]
    assert native.count('call void @dx.op.traceRay') == result.count('call void @dx.op.traceRay')
    assert '%258 = fcmp fast olt float %257, %SpotSample0Max' in result
    assert '%262 = and i1 %258, %261' in result  # Misses excluded from nearest hit.
    assert '%236 = fmul fast float 2.000000e+00, %177' in result
    assert 'i32 157, %dx.types.Handle %256, i32 0, i32 1' in result
    assert '%exitcond446 = icmp eq i32 %263, %165' in result
print('PASS packaged shader hashes and configurable-loop transform when capture supplied')
