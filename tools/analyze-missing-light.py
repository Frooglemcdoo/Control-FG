"""Summarize native-map / RT disagreements without treating visibility as radiance."""
import argparse
from collections import Counter
import importlib.util
import json
import math
from pathlib import Path

spec = importlib.util.spec_from_file_location('probe', Path(__file__).with_name('analyze-map-visibility-probe.py'))
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def summarize(capture, threshold=.25):
    if not math.isfinite(threshold) or not 0 < threshold < 1:
        raise ValueError('Threshold must be finite and between zero and one')
    valid = []
    for record in capture['records']:
        if not record['valid']:
            continue
        sample = probe.sample(record)
        if sample['status'] != 'valid':
            raise ValueError('Invalid matched visibility payload: ' + sample['status'])
        valid.append((record, sample))
    if not valid:
        raise ValueError('No valid matched records')
    targets = [(r, s) for r, s in valid if s['map_minus_rt'] > threshold]
    groups = {}
    for r, s in targets:
        if not r['hit']:
            continue
        key = (r['light_id'], r['instance_index'], r['instance_id'], r['geometry_index'])
        groups.setdefault(key, []).append(r)
    blockers = []
    for key, records in groups.items():
        blockers.append(dict(light_id=key[0], instance_index=key[1], instance_id=key[2],
            geometry_index=key[3], samples=len(records),
            hit_bounds=[[min(r['hit_position'][i] for r in records),
                         max(r['hit_position'][i] for r in records)] for i in range(3)],
            hit_distance_range=[min(r['hit_distance'] for r in records),
                                max(r['hit_distance'] for r in records)]))
    result = dict(schema='control-missing-light-analysis-r201', threshold=threshold,
        valid_samples=len(valid), darker_rt_samples=len(targets),
        reference_hits=sum(r['hit'] for r, s in targets),
        reference_misses=sum(not r['hit'] for r, s in targets),
        partial_native_map_samples=sum(.1 < s['map_visibility'] < .9 for r, s in targets),
        extreme_map_lit_rt_blocked=sum(s['map_visibility'] > .9 and s['rt_visibility'] < .1 for r, s in valid),
        extreme_map_blocked_rt_lit=sum(s['map_visibility'] < .1 and s['rt_visibility'] > .9 for r, s in valid),
        targets_by_light=dict(Counter(r['light_id'] for r, s in targets)),
        blockers=sorted(blockers, key=lambda b: (-b['samples'], b['light_id'], b['instance_index'], b['geometry_index'])),
        limitations='Counts describe sampled visibility, not lost radiance or image area. '
            'Reference hits are not the individual gameplay shadow rays. A reference miss '
            'does not prove a filter error. Partial native visibility can reflect filtering '
            'or authored fade. Geometry identifiers are local to one capture. The map '
            'sidecar and composition evaluate the native map separately.')
    raw = [(r, s) for r, s in targets if 'raw_shadow' in r]
    result['raw_samples_for_darker_rt'] = len(raw)
    result['raw_lit_filtered_blocked'] = sum(r['raw_shadow']['spot_visibility'] > .9 and s['rt_visibility'] < .1 for r, s in raw)
    result['filter_visibility_drop_gt_threshold'] = sum(r['raw_shadow']['spot_visibility'] - s['rt_visibility'] > threshold for r, s in raw)
    result['raw_already_below_map_gt_threshold'] = sum(s['map_visibility'] - r['raw_shadow']['spot_visibility'] > threshold for r, s in raw)
    centers=[(r,s) for r,s in raw if r.get('native_pcf_center') is not None and
        r.get('spot_record',{}).get('shadow_flags',0)&0x302==0x100]
    result['pcf_only_center_samples_for_darker_rt']=len(centers)
    result['map_center_blocked_raw_blocked']=sum(r['native_pcf_center']==0 and r['raw_shadow']['spot_visibility']==0 for r,s in centers)
    result['map_center_lit_raw_blocked']=sum(r['native_pcf_center']==1 and r['raw_shadow']['spot_visibility']==0 for r,s in centers)
    paired=[r['projection_pair'] for r,s in centers if 'projection_pair' in r]
    supported=[p for p in paired if p['supported']]
    result['projection_pairs_for_center_samples']=len(paired)
    result['projection_pairs_unsupported']=len(paired)-len(supported)
    result['clipped_emitter_hit_projection_miss']=sum(p['emitter']['hit'] and not p['projection']['hit'] for p in supported)
    result['both_clipped_paths_hit']=sum(p['emitter']['hit'] and p['projection']['hit'] for p in supported)
    result['clipped_emitter_miss_projection_hit']=sum(not p['emitter']['hit'] and p['projection']['hit'] for p in supported)
    result['both_clipped_paths_miss']=sum(not p['emitter']['hit'] and not p['projection']['hit'] for p in supported)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--threshold', type=float, default=.25)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    text = args.log.read_text(encoding='utf-8-sig')
    # Reuse the existing mode/provenance/payload checks before classification.
    metadata = probe.analyze(text)
    result = summarize(probe.decoder.decode_log(text), args.threshold)
    result.update({k: metadata[k] for k in ('capture_id', 'extent', 'composition', 'map_guard')})
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'blockers'}))


if __name__ == '__main__':
    main()
