"""Exercise binary identities, provenance rejection and hit reconstruction."""
import importlib.util
import math
from pathlib import Path

spec = importlib.util.spec_from_file_location('decoder', Path(__file__).with_name('decode-spot-hit-capture.py'))
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)


def packed(**changes):
    fields = dict(distance=4., instance=0xffffff, instance_id=0xabcdef,
                  primitive=0xfedcba98, u=.25, v=.5, kind=254, geometry=0x12345678,
                  ox=100., oy=20., oz=-30., maximum=5., dx=0., dy=-1., dz=0.,
                  light=7, x=1250, y=900, frame=19, valid=1)
    fields.update(changes)
    return decoder.RECORD.pack(*fields.values())


assert decoder.RECORD.size == 80
hit = decoder.decode(packed(), 19)[0]
assert hit['hit_position'] == [100., 16., -30.]
assert hit['endpoint_gap'] == 1.
assert hit['barycentrics'] == [.25, .25, .5]
assert hit['primitive_index'] == 0xfedcba98  # Must never round through float32.
assert hit['geometry_index'] == 0x12345678
assert hit['instance_id'] == 0xabcdef
miss = packed(kind=0, distance=5., instance=0xffffffff, instance_id=0xffffffff,
              primitive=0xffffffff, geometry=0xffffffff, u=0., v=0.)
assert not decoder.decode(miss, 19)[0]['hit']
assert decoder.decode(bytes(80), 19) == [{'ordinal': 0, 'valid': False}]
for data, frame in [(b'',19), (packed()[:-1],19), (packed()*4097,19), (packed(),20),
                    (packed(),0), (packed(frame=0),19), (packed(valid=2),19),
                    (packed(distance=math.nan),19), (packed(distance=6),19),
                    (packed(dx=1),19), (packed(kind=1),19), (packed(u=.75),19),
                    (packed(instance=0x1000000),19), (packed(kind=0),19)]:
    try:
        decoder.decode(data, frame)
    except ValueError:
        pass
    else:
        raise AssertionError('Invalid capture accepted')
print('PASS capture ABI, exact integer identity, world hit position, misses, bounds and frame validation')
