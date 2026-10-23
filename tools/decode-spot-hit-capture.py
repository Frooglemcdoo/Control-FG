"""Decode completed GPU readback records; never infer identity from screen colors."""
import argparse
import json
import math
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('readback', type=Path)
    parser.add_argument('--frame-tag', required=True, type=int)
    parser.add_argument('--capture-id', required=True,
                        help='Capture/TLAS-generation provenance; IDs are not persistent asset names')
    args = parser.parse_args()
    print(json.dumps({'schema': 'control-spot-hit-capture-v1', 'capture_id': args.capture_id,
                      'records': decode(args.readback.read_bytes(), args.frame_tag)}, indent=2))


if __name__ == '__main__':
    main()
