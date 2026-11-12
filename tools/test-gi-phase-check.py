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
# A bounded target prefix may be structurally complete despite a LATER overflow.
# Preserve the global warning and never convert that result into admission.
rows=[]
def add(kind, data):
    rows.append(e(len(rows)+1, kind, data))
    return len(rows)
for frame in range(100,124):
    for kind in (1,2):
        phase=add(kind,[frame,kind*10,frame,1,1,1,11,20+kind,30,40,1])
        for index in range(8):add(6,[phase,index,1,1000+index+kind])
        call=add(3,[frame,50,1])
        add(4,[call,0,kind*10,phase,0,0])
        add(8,[call])
last=len(rows)
for _ in range(30):add(5,[124,99,88,1,1,0])
rows.append(dict(status=True,first_frame=100,stopped=1,reserved=len(rows)+1,written=len(rows),dropped=1))
r=m.analyze(rows)
assert r['target_prefix']['recording_and_submission_complete']
assert r['target_prefix']['last_required_event']==last
assert r['warnings']==['incomplete_or_overflowed_trace'] and not r['native_suppression_authorized']
assert not m.analyze(rows[:5]+rows[6:])['target_prefix']['recording_and_submission_complete']
assert not m.analyze(rows[:last-1]+rows[last:])['target_prefix']['recording_and_submission_complete']
print('PASS target prefix: late overflow preserved, missing bindings/events rejected, no repeat-capture requirement inferred')


# Revision2 counts observed GI frames, allowing native frame gaps during loading.
for row in rows:
    if row.get('kind') in (1,2,3,5) and row['data'][0]>=102:row['data'][0]+=180
assert not m.analyze(rows)['target_prefix']['recording_and_submission_complete']
rows.insert(-1,e(len(rows),25,[427,1,0,4,2]))
r=m.analyze(rows)
assert r['target_prefix']['recording_and_submission_complete']
assert r['target_prefix']['observed_pairs_complete']
print('PASS revision2 observed-frame window: load gaps retained; legacy semantics preserved')
