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

log = ('ms=1 rt_hit_gpu_recorded generation=7 frame_tag=19 extent=128x128 table_records=2\n'
       'ms=2 rt_hit_gpu_complete generation=7 frame_tag=19 unknown=0 translated=1 null=1 invalid_layout=0 valid_rays=1 hits=1 misses=0 invalid_records=0 gpu_completion_confirmed=1 gpu_hit_capture=1\n'
       'ms=3 rt_hit_gpu_record index=0 bytes='+packed(x=1,y=1).hex()+'\n'
       'ms=4 rt_hit_gpu_export_complete records_examined=4096 frame_tag=19\n')
assert decoder.decode_log(log)['records'][0]['primitive_index']==0xfedcba98
for bad in [log.replace('gpu_completion_confirmed=1','gpu_completion_confirmed=0'),
            log.replace('unknown=0','unknown=1'),log.replace('valid_rays=1','valid_rays=2'),
            log.replace('index=0','index=1'),log.replace('frame_tag=19','frame_tag=20'),
            log.replace('ms=4 rt_hit_gpu_export_complete','ms=4 interrupted'),
            log+'ms=5 rt_hit_gpu_record index=0 bytes='+packed(x=1,y=1).hex()+'\n',
            log+'ms=5 rt_hit_gpu_failed reason=test\n']:
    try:decoder.decode_log(bad)
    except ValueError:pass
    else:raise AssertionError('Invalid capture log accepted')
print('PASS complete-log decoding: fenced provenance, record counts, exact grid pixels, partial/duplicate/replayed exports rejected')

# R133 metadata is optional only for old logs, mandatory when declared.
import struct
context = struct.pack('<32f', *range(32)).hex()
context_log = log.replace('table_records=2', 'table_records=2 light_context_bytes=128').replace(
    packed(x=1,y=1).hex(), packed(x=1,y=1).hex()+' light_context='+context)
result = decoder.decode_log(context_log)['records'][0]
assert result['light_context']['cone_view']==[0,1,2]
assert result['light_context']['near_distance']==3
assert result['light_context']['emitter_view']==[4,5,6]
assert result['light_context']['direction_view']==[8,9,10]
assert result['light_context']['radius']==14
assert result['light_context']['view_to_world']==[[16,17,18,19],[20,21,22,23],[24,25,26,27],[28,29,30,31]]
for bad in [context_log.replace(' light_context='+context,''),
            context_log.replace(' light_context_bytes=128',''),
            context_log.replace('light_context_bytes=128','light_context_bytes=64'),
            context_log.replace(context,context[:-2])]:
    try:decoder.decode_log(bad)
    except ValueError:pass
    else:raise AssertionError('Missing, undeclared or truncated light context accepted')
print('PASS R133 light context field offsets, versioning, completeness and R132 compatibility')
