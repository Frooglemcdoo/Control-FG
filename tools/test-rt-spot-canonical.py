"""Verify the shipped canonical binary and optional captured-library binding compatibility."""
import base64
import hashlib
import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root/'assets/rt-spot/manifest.json').read_text())
runtime = (root/'src/rt_spot_runtime.h').read_text()
for name, expected in manifest['outputs'].items():
    data = base64.b64decode((root/'assets/rt-spot'/f'{name}.b64').read_text())
    assert len(data) == expected['bytes']
    assert hashlib.sha256(data).hexdigest() == expected['sha256']
    assert expected['sha256'] in runtime, f'Runtime rejects packaged {name}'
assert manifest['outputs']['rt_spot_full.dxil']['sha256'] == 'c1f3f1b7b8845dd85224c6c41e7c8d10c6cd0fc793328e7168766563352707a5'
assert 'rt_spot_multi.dxil' not in manifest['outputs']
assert 'multiRayBytes' not in runtime
assert 'Matches(shader,kRaySha)) { replacementBytes=&rayBytes;' in runtime
assert 'Matches(shader,kMultiRaySha)) { replacementBytes=&rayBytes;' in runtime

if len(sys.argv) == 3:
    single, multi = [Path(p).read_text() for p in sys.argv[1:]]
    def bindings(source):
        table = source.split('; Resource Bindings:',1)[1].split('target datalayout',1)[0]
        result = {}
        for line in table.splitlines():
            parts = line.lstrip('; ').split()
            if len(parts) >= 6 and re.fullmatch(r'(CB|S|T|U)\d+',parts[4]):
                # The reflection ordinal is not a shader register; ignore that column.
                result[parts[0]] = tuple(parts[1:4]+parts[5:])
        assert result
        return result
    a,b = bindings(single),bindings(multi)
    assert all(b.get(name) == layout for name,layout in a.items())
    assert set(b)-set(a) == {'deferredlight_constants'}
    # Same payload, structured-light data layouts and hit-root constants.
    for type_name in ('ShadowHitData','DeferredLightSpot','DeferredLightPoint','RaytracingHitRootConstants'):
        pattern = rf'^%struct\.{type_name} = type .*'
        assert re.search(pattern,single,re.M)[0] == re.search(pattern,multi,re.M)[0]
    print('PASS captured native bindings: canonical library is a subset; payload and light/hit layouts match')
print('PASS packaged canonical shader hash and shared replacement selection for both native variants')
