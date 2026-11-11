import importlib.util
from pathlib import Path
spec = importlib.util.spec_from_file_location('phase', Path(__file__).with_name('analyze-gi-phase-check.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
def e(seq, kind, values):
    return dict(seq=seq, kind=kind, data=values + [0]*(12-len(values)))
rows = [e(1, 1, [100, 10, 1, 1, 1, 1, 11, 20, 30, 40, 1]),
        e(2, 2, [100, 10, 1, 1, 1, 1, 11, 21, 30, 40, 1]),
        e(3, 3, [100, 50, 1]), e(4, 4, [3, 0, 10, 1, 0, 0]),
        e(5, 4, [3, 0, 10, 2, 1, 0]), e(6, 8, [3]),
        dict(status=True, stopped=1, reserved=6, written=6, dropped=0)]
r=m.analyze(rows)
assert not r['warnings'] and r['frame_relationships'][0]['relationship']=='same_recording_ray_before_evaluator'
assert r['native_suppression_authorized'] is False
rows[4]['data'][1]=1
assert m.analyze(rows)['frame_relationships'][0]['relationship']=='same_execute_call_distinct_lists'
rows[4]['data'][5]=1
assert 'recording_marker_overflow' in m.analyze(rows)['warnings']
rows[-1]['dropped']=1
assert 'incomplete_or_overflowed_trace' in m.analyze(rows)['warnings']
assert 'phase_submission_missing_or_repeated' in m.analyze(rows[:4]+rows[-1:])['warnings']
assert 'no_gi_phases' in m.analyze([])['warnings']
print('PASS phase parser: recording/submission distinction, incomplete evidence and no false admission')
