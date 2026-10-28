"""Thresholds, reference misses, invalid payloads and capture-local grouping."""
import importlib.util
from pathlib import Path
spec = importlib.util.spec_from_file_location('analysis', Path(__file__).with_name('analyze-missing-light.py'))
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)

def record(light, native, rt, hit=True):
    return dict(valid=True, light_id=light, pixel=[0, 0],
        native_map=[native, 1-native, light+1, 0], composition=dict(visibility=[rt,0,0,0]),
        hit=hit, instance_index=5, instance_id=0, geometry_index=2,
        hit_position=[1,2,3], hit_distance=2)

records = [record(1,1,0), record(1,.5,0,False), record(2,1,0),
           record(1,.5,.25), record(1,0,1), dict(valid=False)]
result = analysis.summarize(dict(records=records))
assert result['valid_samples'] == 5 and result['darker_rt_samples'] == 3
assert result['reference_hits'] == 2 and result['reference_misses'] == 1
assert result['partial_native_map_samples'] == 1
assert result['extreme_map_lit_rt_blocked'] == 2
assert result['extreme_map_blocked_rt_lit'] == 1
assert len(result['blockers']) == 2  # Same geometry, different selected lights.
assert result['targets_by_light'] == {1:2, 2:1}
for threshold in [0,1,-1,float('nan'),float('inf')]:
    try: analysis.summarize(dict(records=records), threshold)
    except ValueError: pass
    else: raise AssertionError('Invalid threshold accepted')
bad = record(1,1,0); bad['native_map'][2] = 99
for capture in [dict(records=[bad]), dict(records=[dict(valid=False)])]:
    try: analysis.summarize(capture)
    except ValueError: pass
    else: raise AssertionError('Invalid capture accepted')
print('PASS missing-light analysis: boundaries, misses, grouping and invalid input')
records[0]['raw_shadow'] = dict(spot_visibility=1)
records[1]['raw_shadow'] = dict(spot_visibility=0)
records[2]['raw_shadow'] = dict(spot_visibility=.5)
result=analysis.summarize(dict(records=records))
assert result['raw_samples_for_darker_rt']==3
assert result['raw_lit_filtered_blocked']==1
assert result['filter_visibility_drop_gt_threshold']==2
assert result['raw_already_below_map_gt_threshold']==2
print('PASS raw-versus-filtered classification: measured raw values required; overlapping contributions retained')
