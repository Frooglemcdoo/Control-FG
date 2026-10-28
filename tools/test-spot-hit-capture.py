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

signal = struct.pack('<4I12f', 1, 1, 7, 65535, .25, .75, 0, 0, 8, 4, 2, 1, 16, 8, 4, 1).hex()
# The fixture's actual selected light ID is authoritative.
light = decoder.decode_log(context_log)['records'][0]['light_id']
signal = struct.pack('<4I12f', 1, 1, light, 65535, .25, .75, 0, 0, 8, 4, 2, 1, 16, 8, 4, 1).hex()
signal_log = context_log.replace('light_context_bytes=128', 'light_context_bytes=128 signal_bytes=64').replace(' light_context='+context, ' light_context='+context+' signal='+signal)
result = decoder.decode_log(signal_log)['records'][0]['composition']
assert result['visibility'][0] == .25 and result['diffuse'][0] == 8 and result['specular'][0] == 16
for bad in [signal_log.replace(' signal='+signal, ''), signal_log.replace('signal_bytes=64','signal_bytes=32'),
            signal_log.replace(signal, '00000000'+signal[8:]), signal_log.replace(signal, signal[:32]+'0000807f'+signal[40:])]:
    try: decoder.decode_log(bad)
    except ValueError: pass
    else: raise AssertionError('Invalid composition capture accepted')
print('PASS R145 signal decoding: HDR preserved, pixel provenance, finite visibility, complete ABI')

separate = signal_log.replace('signal_bytes=64', 'signal_bytes=64 signal_generation=987 signal_present=42 captured_present=42')
assert decoder.decode_log(separate)['records'][0]['composition']['diffuse'][0] == 8
for bad in [separate.replace('signal_generation=987','signal_generation=0'),separate.replace('signal_present=42','signal_present=43')]:
    try: decoder.decode_log(bad)
    except ValueError: pass
    else: raise AssertionError('Mismatched composition submission accepted')
print('PASS R146 separate generation and matching captured frame')

# Paired composition ABI preserves signed deltas and rejects mismatched pairs.
before = struct.pack('<4I12f', 1, 1, light, 65535, .25, .75, 0, 0, 10, 3, 1, 1, 12, 7, 3, 1).hex()
paired = signal_log.replace('signal_bytes=64', 'signal_bytes=128').replace('signal='+signal, 'signal='+signal+before)
c = decoder.decode_log(paired)['records'][0]['composition']
assert c['diffuse_delta'] == [-2, 1, 1, 0] and c['specular_delta'] == [4, 1, 1, 0]
for bad in [paired.replace('signal_bytes=128','signal_bytes=64'), paired.replace(before, '00000000'+before[8:]), paired.replace(before, before[:32]+'0000807f'+before[40:])]:
    try: decoder.decode_log(bad)
    except ValueError: pass
    else: raise AssertionError('Invalid paired composition accepted')
print('PASS R150 paired composition: signed pass deltas, matching pixel/light/visibility, exact ABI')

# R161 retains old signal ABI and validates one full spotlight record per ray.
spot = bytearray(336)
struct.pack_into('<4I', spot, 0, light, light+1, 320, 1)
struct.pack_into('<16f', spot, 96, *[float(i%5==0) for i in range(16)])
line = ' rt_hit_gpu_light index=0 bytes='+spot.hex()
projection_log = paired.replace('signal_bytes=128', 'signal_bytes=128 spot_record_bytes=336')
projection_log = projection_log.replace('rt_hit_gpu_export_complete', line+'\nms=0 rt_hit_gpu_export_complete')
assert decoder.decode_log(projection_log)['records'][0]['spot_record']['projection_rows'][0]==[1,0,0,0]
for bad in [projection_log.replace(line, ''), projection_log.replace(line,line+'\n'+line),
            projection_log.replace(spot.hex(),spot[:8].hex()+'00000000'+spot[12:].hex())]:
    try: decoder.decode_log(bad)
    except ValueError: pass
    else: raise AssertionError('Invalid spotlight provenance accepted')
print('PASS full spotlight records: projection decode, missing/duplicate/invalid stride rejected')

# Private map ABI cannot be confused with ordinary diffuse radiance.
map_hex=struct.pack('<4f',.25,.75,float(light+1),0).hex()
sidecar=projection_log.replace('spot_record_bytes=336', 'spot_record_bytes=336 map_bytes=16 map_generation=12 map_present=42 captured_present=42').replace(line,line+' map='+map_hex)
assert decoder.decode_log(sidecar)['records'][0]['native_map']==[.25,.75,float(light+1),0]
for bad in [sidecar.replace('map_present=42','map_present=43'),sidecar.replace('map_generation=12','map_generation=0'),sidecar.replace(' map='+map_hex,''),sidecar.replace('map_bytes=16','map_bytes=0'),sidecar.replace('map_bytes=16','map_bytes=32')]:
    try:decoder.decode_log(bad)
    except ValueError:pass
    else:raise AssertionError('Invalid private map provenance accepted')
print('PASS R196 private map payload, frame identity and missing/malformed data rejection')

# R198: exact same ray in two geometry-only probes; only native misses qualify.
base_hit=packed(x=1,y=1)
base_miss=packed(x=1,y=1,kind=0,distance=5.,instance=0xffffffff,instance_id=0xffffffff,primitive=0xffffffff,geometry=0xffffffff,u=0.,v=0.)
probe_log=sidecar.replace('map_bytes=16','map_bytes=16 eligibility_bytes=160').replace(base_hit.hex(),base_miss.hex()).replace('hits=1 misses=0','hits=0 misses=1')
probe_line='ms=3 rt_hit_gpu_eligibility index=0 bytes='+(base_hit+base_hit).hex()
probe_log=probe_log.replace('ms=0 rt_hit_gpu_export_complete',probe_line+'\nms=0 rt_hit_gpu_export_complete')
item=decoder.decode_log(probe_log)['records'][0]
assert item['eligibility']['mask1_opaque']['hit'] and item['eligibility']['mask255_opaque']['hit']
for bad in [probe_log.replace((base_hit+base_hit).hex(),(base_hit+base_miss).hex()),probe_log.replace(probe_line,''),probe_log.replace(probe_line,probe_line+'\n'+probe_line),
            probe_log.replace('eligibility_bytes=160','eligibility_bytes=0'),
            probe_log.replace((base_hit+base_hit).hex(),(packed(x=1,y=1,frame=20)+base_miss).hex()),
            probe_log.replace((base_hit+base_hit).hex(),(packed(x=1,y=1,dx=1)+base_miss).hex()),
            probe_log.replace((base_hit+base_hit).hex(),(packed(x=1,y=1,distance=6)+base_miss).hex())]:
    try:decoder.decode_log(bad)
    except ValueError:pass
    else:raise AssertionError('Invalid eligibility provenance accepted')
print('PASS R198 eligibility records: exact ray/frame/pixel match, sparse completeness, duplicate/truncated/invalid hit rejection')
spec=importlib.util.spec_from_file_location('blockers',Path(__file__).with_name('analyze-window-blockers.py'))
blockers=importlib.util.module_from_spec(spec);spec.loader.exec_module(blockers)
assert blockers.classify(item)=='same_mask_geometry_hit'
item['eligibility']['mask1_opaque']['hit']=False;item['eligibility']['mask255_opaque']['hit']=True
assert blockers.classify(item)=='wider_mask_only_hit'
item['eligibility']['mask255_opaque']['hit']=False
assert blockers.classify(item)=='both_geometry_queries_miss'
try:blockers.classify({'hit':False})
except ValueError:pass
else:raise AssertionError('Legacy miss accepted as new probe evidence')
print('PASS geometry-probe interpretation: separate opacity/mask/no-hit outcomes; legacy evidence rejected')

raw_payload=struct.pack('<4I4f',1,1,19,7,.75,.25,4.,50000.).hex()
raw_line='ms=3 rt_hit_gpu_raw index=0 bytes='+raw_payload
raw_log=sidecar.replace('map_bytes=16','map_bytes=16 raw_bytes=32').replace('ms=0 rt_hit_gpu_export_complete',raw_line+'\nms=0 rt_hit_gpu_export_complete')
assert decoder.decode_log(raw_log)['records'][0]['raw_shadow']['spot_visibility']==.75
for bad in [raw_log.replace(raw_line+'\n',''),raw_log.replace(raw_line,raw_line+'\n'+raw_line),
            raw_log.replace('raw_bytes=32','raw_bytes=0'),raw_log.replace('raw_bytes=32','raw_bytes=16'),
            raw_log.replace(raw_payload,struct.pack('<4I4f',1,1,20,7,.75,.25,4.,50000.).hex()),
            raw_log.replace(raw_payload,struct.pack('<4I4f',1,1,19,8,.75,.25,4.,50000.).hex()),
            raw_log.replace(raw_payload,struct.pack('<4I4f',2,1,19,7,.75,.25,4.,50000.).hex()),
            raw_log.replace(raw_payload,struct.pack('<4I4f',1,1,19,7,float('nan'),.25,4.,50000.).hex()),
            raw_log.replace(raw_payload,struct.pack('<4I4f',1,1,19,7,1.25,.25,4.,50000.).hex()),
            raw_log.replace(raw_payload,struct.pack('<4I4f',1,1,19,7,.75,.25,-1.,50000.).hex())]:
    try:decoder.decode_log(bad)
    except ValueError:pass
    else:raise AssertionError('Invalid raw shadow capture accepted')
print('PASS R201 raw shadow: exact values; missing, duplicate, stale, wrong-light/pixel and invalid values rejected')
