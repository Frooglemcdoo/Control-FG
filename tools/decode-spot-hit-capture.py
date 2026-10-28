"""Decode completed GPU readback records; never infer identity from screen colors."""
import argparse
import json
import math
import re
import struct
from pathlib import Path

RECORD = struct.Struct('<fIIIffII3ff3f5I')
MAX_RECORDS = 4096


def decode(data, frame_tag):
    if not 0 < frame_tag <= 0xffffffff:
        raise ValueError('A nonzero captured frame tag is required')
    if not data or len(data) % RECORD.size or len(data) > MAX_RECORDS * RECORD.size:
        raise ValueError('Invalid capture byte length')
    results = []
    for ordinal, values in enumerate(RECORD.iter_unpack(data)):
        distance, instance, instance_id, primitive, u, v, kind, geometry = values[:8]
        origin, maximum, direction = values[8:11], values[11], values[12:15]
        light, x, y, frame, valid = values[15:]
        if valid == 0:
            results.append({'ordinal': ordinal, 'valid': False})
            continue
        if valid != 1 or frame != frame_tag:
            raise ValueError(f'Record {ordinal}: stale frame or invalid status')
        if not all(math.isfinite(n) for n in (*origin, *direction, maximum, distance, u, v)):
            raise ValueError(f'Record {ordinal}: nonfinite geometry')
        if maximum <= 0 or not 0 <= distance <= maximum or abs(sum(n*n for n in direction)-1) > .0002:
            raise ValueError(f'Record {ordinal}: invalid ray segment')
        if kind not in (0, 254, 255):
            raise ValueError(f'Record {ordinal}: unsupported hit kind')
        result = {'ordinal': ordinal, 'valid': True, 'pixel': [x, y],
                  'frame_tag': frame, 'light_id': light, 'hit': kind != 0,
                  'origin': origin, 'direction': direction, 't_max': maximum}
        if kind:
            if min(u, v) < -.00001 or u+v > 1.00001:
                raise ValueError(f'Record {ordinal}: invalid triangle coordinates')
            if instance > 0xffffff or instance_id > 0xffffff:
                raise ValueError(f'Record {ordinal}: invalid 24-bit instance field')
            result.update(instance_index=instance, instance_id=instance_id,
                          geometry_index=geometry, primitive_index=primitive,
                          barycentrics=[1-u-v, u, v], hit_kind=kind,
                          hit_distance=distance, endpoint_gap=maximum-distance,
                          hit_position=[o+distance*d for o, d in zip(origin, direction)])
        else:
            if distance != maximum or any(n != 0xffffffff for n in (instance, instance_id, primitive, geometry)):
                raise ValueError(f'Record {ordinal}: miss retained stale identity')
        results.append(result)
    return results


def decode_log(text):
    """Accept only a complete, fenced R128 capture with every valid record present."""
    def fields(line):
        return {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)(?:\s|$)', line)}
    recorded = [line for line in text.splitlines() if ' rt_hit_gpu_recorded ' in line]
    completed = [line for line in text.splitlines() if ' rt_hit_gpu_complete ' in line]
    exported = [line for line in text.splitlines() if ' rt_hit_gpu_export_complete ' in line]
    if len(recorded) != 1 or len(completed) != 1 or len(exported) != 1 or ' rt_hit_gpu_failed ' in text:
        raise ValueError('Capture did not complete exactly once')
    begin, finish, end = fields(recorded[0]), fields(completed[0]), fields(exported[0])
    if not (text.index(recorded[0]) < text.index(completed[0]) < text.index(exported[0])):
        raise ValueError('Capture markers are out of order')
    if (finish.get('gpu_completion_confirmed') != 1 or finish.get('gpu_hit_capture') != 1 or
        finish.get('unknown') != 0 or finish.get('invalid_layout') != 0 or finish.get('invalid_records') != 0 or
        finish.get('generation') != begin.get('generation') or
        not begin.get('frame_tag') or finish.get('frame_tag') != begin['frame_tag'] or end.get('frame_tag') != begin['frame_tag'] or
        end.get('records_examined') != MAX_RECORDS or
        finish.get('translated', -1) + finish.get('null', -1) != begin.get('table_records')):
        raise ValueError('Capture GPU validation or provenance failed')
    if 'signal_generation' in begin or 'signal_present' in begin:
        if not begin.get('signal_generation') or begin.get('signal_present') != begin.get('captured_present'):
            raise ValueError('Composition submission/frame provenance mismatch')
    extent = re.search(r'extent=(\d+)x(\d+)', recorded[0])
    if not extent: raise ValueError('Missing native image extent')
    width, height = map(int, extent.groups())
    if not 0 < width <= 16384 or not 0 < height <= 16384: raise ValueError('Invalid native image extent')
    data = bytearray(MAX_RECORDS * RECORD.size)
    seen = set()
    contexts = {}
    signals = {}
    signal_bytes = begin.get("signal_bytes", 0)
    if signal_bytes not in (0, 64, 128): raise ValueError("Unsupported composition signal ABI")
    context_bytes = begin.get("light_context_bytes", 0)
    if context_bytes not in (0, 128): raise ValueError("Unsupported light context ABI")
    for line in text.splitlines():
        if ' rt_hit_gpu_record ' not in line: continue
        if not text.index(completed[0]) < text.index(line) < text.index(exported[0]):
            raise ValueError('Record is outside the completed export')
        match = re.search(r'index=(\d+) bytes=([0-9a-fA-F]{160})(?: light_context=([0-9a-fA-F]{256}))?(?: signal=([0-9a-fA-F]{128}(?:[0-9a-fA-F]{128})?))?$', line)
        if not match: raise ValueError('Malformed capture record')
        index = int(match[1])
        if index >= MAX_RECORDS or index in seen: raise ValueError('Duplicate or out-of-range record')
        if bool(match[3]) != bool(context_bytes): raise ValueError('Missing or unexpected light context')
        if bool(match[4]) != bool(signal_bytes): raise ValueError('Missing or unexpected composition signal')
        if match[4]:
            raw_signal = bytes.fromhex(match[4])
            if len(raw_signal) != signal_bytes: raise ValueError('Composition signal size differs from ABI')
            signals[index] = struct.unpack('<4I12f' + ('4I12f' if signal_bytes == 128 else ''), raw_signal)
        if match[3]: contexts[index] = struct.unpack('<32f', bytes.fromhex(match[3]))
        seen.add(index); data[index*RECORD.size:(index+1)*RECORD.size] = bytes.fromhex(match[2])
    spot_records = {}; map_records = {}
    map_bytes = begin.get('map_bytes', 0)
    if map_bytes not in (0,16): raise ValueError('Unsupported native map ABI')
    if map_bytes and (not begin.get('map_generation') or begin.get('map_present') != begin.get('captured_present')):
        raise ValueError('Native map frame provenance mismatch')
    spot_bytes = begin.get('spot_record_bytes', 0)
    if spot_bytes not in (0, 336): raise ValueError('Unsupported spotlight record ABI')
    for line in text.splitlines():
        if ' rt_hit_gpu_light ' not in line: continue
        if not spot_bytes or not text.index(completed[0]) < text.index(line) < text.index(exported[0]):
            raise ValueError('Unexpected spotlight record or invalid export ordering')
        match = re.search(r'index=(\d+) bytes=([0-9a-fA-F]{672})(?: map=([0-9a-fA-F]{32}))?$', line)
        if not match: raise ValueError('Malformed spotlight record')
        index = int(match[1])
        if index not in seen or index in spot_records: raise ValueError('Unmatched or duplicate spotlight record')
        if bool(match[3]) != bool(map_bytes): raise ValueError('Missing or unexpected native map payload')
        if map_bytes: map_records[index] = list(struct.unpack('<4f', bytes.fromhex(match[3])))
        spot_records[index] = bytes.fromhex(match[2])
    if spot_bytes and spot_records.keys() != seen:
        raise ValueError('Missing spotlight records')
    results = decode(data, begin['frame_tag'])
    valid = [record for record in results if record['valid']]
    hits = sum(record['hit'] for record in valid)
    if len(seen) != len(valid) or len(valid) != finish.get('valid_rays') or hits != finish.get('hits') or len(valid)-hits != finish.get('misses'):
        raise ValueError('Missing records or inconsistent capture totals')
    eligibility_bytes=begin.get('eligibility_bytes',0)
    if eligibility_bytes not in (0,160) or (eligibility_bytes and map_bytes!=16):
        raise ValueError('Unsupported eligibility ABI')
    probes={}
    for line in text.splitlines():
        if ' rt_hit_gpu_eligibility ' not in line: continue
        if not eligibility_bytes or not text.index(completed[0]) < text.index(line) < text.index(exported[0]):
            raise ValueError('Unexpected eligibility record or export ordering')
        match=re.search(r'index=(\d+) bytes=([0-9a-fA-F]{320})$',line)
        if not match: raise ValueError('Malformed eligibility record')
        index=int(match[1])
        if index not in seen or index in probes: raise ValueError('Duplicate or unmatched eligibility record')
        raw=bytes.fromhex(match[2]);items=[]
        for bank in range(2):
            part=raw[bank*80:(bank+1)*80]
            if part[32:]!=data[index*80+32:(index+1)*80]:
                raise ValueError('Eligibility ray/frame/pixel provenance mismatch')
            item=decode(part,begin['frame_tag'])[0]
            if not item['valid']: raise ValueError('Invalid eligibility probe')
            item['ordinal']=index;items.append(item)
        if items[0]['hit'] and (not items[1]['hit'] or items[1]['hit_distance']>items[0]['hit_distance']+1e-4*max(1,items[0]['t_max'])):
            raise ValueError('Geometry mask superset is inconsistent')
        probes[index]=dict(mask1_opaque=items[0],mask255_opaque=items[1])
    expected={r['ordinal'] for r in valid if not r['hit']} if eligibility_bytes else set()
    if set(probes)!=expected: raise ValueError('Eligibility probes missing or attached to a native hit')
    for record in valid:
        index = record['ordinal']
        if index in probes: record['eligibility']=probes[index]
        if map_bytes: record['native_map'] = map_records[index]
        if context_bytes:
            values = contexts[index]
            # Width/falloff fields are retained as raw floats; interpretation is
            # separate from capture validity. Never use them to alter a ray here.
            record['light_context'] = dict(cone_view=list(values[:3]), near_distance=values[3],
                emitter_view=list(values[4:7]), far_distance=values[7], direction_view=list(values[8:11]),
                far_width=values[11], falloff=list(values[12:14]), radius=values[14], sin_half_cone_squared=values[15],
                view_to_world=[list(values[16+i*4:20+i*4]) for i in range(4)])
        if signal_bytes:
            signal = signals[index]
            if list(signal[:2]) != record['pixel'] or signal[2] != record['light_id']:
                raise ValueError('Composition pixel/light differs from captured ray')
            if not all(math.isfinite(x) for x in signal[4:]) or not -.001 <= signal[4] <= 1.001:
                raise ValueError('Invalid composition signal')
            record['composition'] = dict(point_light_id=signal[3], visibility=list(signal[4:8]),
                                         diffuse=list(signal[8:12]), specular=list(signal[12:16]))
            if signal_bytes == 128:
                before = signal[16:]
                if signal[:8] != before[:8]:
                    raise ValueError('Composition pair pixel/light/visibility mismatch')
                composition = record['composition']
                composition['diffuse_before'] = list(before[8:12])
                composition['specular_before'] = list(before[12:16])
                composition['diffuse_delta'] = [a-b for a,b in zip(signal[8:12],before[8:12])]
                composition['specular_delta'] = [a-b for a,b in zip(signal[12:16],before[12:16])]
        if spot_bytes:
            raw = spot_records[index]
            light, count, stride, valid_record = struct.unpack_from('<4I', raw)
            if valid_record != 1 or stride != 320 or light != record['light_id'] or light >= count:
                raise ValueError('Invalid spotlight record provenance or stride')
            # Keep flag bits as integers, never reinterpret the full record as floats.
            matrix = struct.unpack_from('<16f', raw, 16 + 80)
            if not all(math.isfinite(v) for v in matrix):
                raise ValueError('Nonfinite spotlight projection')
            record['spot_record'] = dict(light_id=light, count=count, stride=stride,
                raw_hex=raw[16:].hex(), projection_rows=[list(matrix[i:i+4]) for i in range(0,16,4)],
                shadow_flags=struct.unpack_from('<I', raw, 16+304)[0])
        if record['pixel'] != [(2*(index%64)+1)*width//128, (2*(index//64)+1)*height//128]:
            raise ValueError('Record does not match its native sample pixel')
    return {'schema': 'control-spot-hit-capture-v1',
            'capture_id': f"generation-{begin['generation']}-frame-{begin['frame_tag']}",
            'extent': [width, height], 'records': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('readback', type=Path)
    parser.add_argument('--log', action='store_true', help='Decode a completed Lighting-R128 session log')
    parser.add_argument('--frame-tag', type=int)
    parser.add_argument('--capture-id',
                        help='Capture/TLAS-generation provenance; IDs are not persistent asset names')
    args = parser.parse_args()
    if args.log:
        print(json.dumps(decode_log(args.readback.read_text(encoding='utf-8-sig')), indent=2))
        return
    if args.frame_tag is None or not args.capture_id:
        parser.error('Binary input requires --frame-tag and --capture-id')
    print(json.dumps({'schema': 'control-spot-hit-capture-v1', 'capture_id': args.capture_id,
                      'records': decode(args.readback.read_bytes(), args.frame_tag)}, indent=2))


if __name__ == '__main__':
    main()
