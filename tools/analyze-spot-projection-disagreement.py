"""Measure captured map/RT path differences; never infer visibility on an untraced path."""
import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path

import numpy as np

spec = importlib.util.spec_from_file_location('missing', Path(__file__).with_name('analyze-missing-light.py'))
missing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(missing)


def geometry(record):
    ctx = record['light_context']
    matrix = np.asarray(record['spot_record']['projection_rows'], dtype=float)
    transform = np.asarray(ctx['view_to_world'], dtype=float)
    if matrix.shape != (4, 4) or transform.shape != (4, 4):
        raise ValueError('Expected 4x4 matrices')
    if not np.isfinite(matrix).all() or not np.isfinite(transform).all():
        raise ValueError('Nonfinite matrix')
    # The native shader uses row-vector projection. A perspective apex has
    # homogeneous x=y=w=0; z is deliberately NOT constrained to zero.
    system = matrix[:3, [0, 1, 3]].T
    if np.linalg.cond(system) > 1e8:
        raise ValueError('Projection has no stable finite perspective apex')
    apex_view = np.linalg.solve(system, -matrix[3, [0, 1, 3]])
    apex = (np.r_[apex_view, 1] @ transform)[:3]
    cone = (np.r_[ctx['cone_view'], 1] @ transform)[:3]
    emitter = (np.r_[ctx['emitter_view'], 1] @ transform)[:3]
    axis = np.asarray(ctx['direction_view']) @ transform[:3, :3]
    if not np.isfinite(axis).all() or np.linalg.norm(axis) < 1e-12:
        raise ValueError('Invalid cone axis')
    axis /= np.linalg.norm(axis)
    origin = np.asarray(record['origin'])
    direction = np.asarray(record['direction'])
    projection_delta = apex-origin
    if np.linalg.norm(projection_delta) < 1e-12:
        raise ValueError('Receiver is at projection apex')
    projection_direction = projection_delta/np.linalg.norm(projection_delta)
    result = dict(apex_cone_residual=float(np.linalg.norm(apex-cone)),
        apex_emitter_distance=float(np.linalg.norm(apex-emitter)),
        angle_degrees=float(np.degrees(np.arccos(np.clip(projection_direction @ direction, -1, 1)))))
    if record['hit']:
        hit = np.asarray(record['hit_position'])
        result['hit_plane_margin'] = float((hit-cone) @ axis-ctx['near_distance'])
        result['hit_distance'] = record['hit_distance']
        denominator = float(projection_direction @ axis)
        if abs(denominator) > 1e-8:
            distance = float((hit-origin) @ axis)/denominator
            if 0 <= distance <= np.linalg.norm(projection_delta):
                result['path_separation_at_hit_depth'] = float(np.linalg.norm(origin+distance*projection_direction-hit))
    if not all(np.isfinite(v) for v in result.values()):
        raise ValueError('Nonfinite geometry result')
    return result


def summarize(capture, threshold=.25):
    missing.summarize(capture, threshold)  # Validate matched payloads and threshold.
    groups = {}
    for r in capture['records']:
        if not r['valid']:
            continue
        if (r.get('native_pcf_center') != 1 or
                r.get('raw_shadow', {}).get('spot_visibility') != 0 or
                r.get('spot_record', {}).get('shadow_flags', 0) & 0x302 != 0x100 or
                missing.probe.sample(r)['map_minus_rt'] <= threshold):
            continue
        groups.setdefault(r['light_id'], []).append((r, geometry(r)))
    lights = []
    for light, pairs in sorted(groups.items()):
        names = sorted({k for _, item in pairs for k in item})
        metrics = {}
        for name in names:
            values = [item[name] for _, item in pairs if name in item]
            metrics[name] = dict(count=len(values), min_median_max=np.quantile(values, [0,.5,1]).tolist())
        lights.append(dict(light_id=light, samples=len(pairs), reference_hits=sum(r['hit'] for r,_ in pairs),
            hits_before_authored_near_plane=sum(item.get('hit_plane_margin', 0) < -1e-4 for _,item in pairs),
            hit_kinds=dict(Counter(r.get('hit_kind', 'miss') for r,_ in pairs)), metrics=metrics))
    return dict(schema='control-spot-projection-disagreement-v1', capture_id=capture['capture_id'], lights=lights,
        limitations='Sampled visibility is not radiance or screen area. Reference origins include RT receiver bias; '
        'the native map receiver can differ. Path separation is a geometric construction at equal cone-axis depth, '
        'not a traced projection ray or proof of map caster coverage. Hits and light IDs are capture-local. '
        'Apex agreement validates geometry, not an alternative rendering implementation.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    text = args.log.read_text(encoding='utf-8-sig')
    missing.probe.analyze(text)
    result = summarize(missing.probe.decoder.decode_log(text))
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
