"""R427 observation report. Never converts sampling into scene ownership."""
import argparse
import json
from pathlib import Path

def analyze(rows):
    events = {r['seq']: r for r in rows if 'seq' in r}
    samples = [r for r in events.values() if r.get('kind') == 22]
    details = {r['data'][0]: r['data'] for r in events.values() if r.get('kind') == 23}
    heaps = {(r['data'][0], r['data'][1]): r['data'] for r in events.values() if r.get('kind') == 24}
    installs = [r for r in events.values() if r.get('kind') == 25]
    warnings = []
    if len(installs) != 1 or installs[0]['data'][:3] != [427, 1, 0]:
        warnings.append('scene_hooks_not_confirmed')
    status = next((r for r in reversed(rows) if r.get('status')), {})
    if not status.get('stopped') or status.get('dropped', 1) or status.get('reserved') != status.get('written'):
        warnings.append('trace_incomplete_or_overflowed')
    if len(events) != sum('seq' in r for r in rows) or sorted(events) != list(range(1, len(events)+1)):
        warnings.append('missing_or_duplicate_sequence')
    def summary(s):
        d = s['data']; raw = details.get(s['seq']); heap = heaps.get((s['seq'], 0))
        phase = events.get(d[0], {})
        return dict(sequence=s['seq'], parent=d[0], frame=d[1], point=d[2], serial_before=d[3], serial_after=d[4],
                    active=d[5:8], valid_mask=d[8], fault=d[9], thread=d[10], joined=bool(d[11]),
                    concurrent_change=d[3]!=d[4], context=raw, resource_heap=heap,
                    phase_heap_matches=(heap[3] == phase['data'][8]) if heap and phase.get('kind') in (1,2) else None)
    parsed = [summary(s) for s in samples]
    if any(s['fault'] for s in parsed): warnings.append('native_metadata_read_fault')
    if any(s['context'] is None or s['resource_heap'] is None for s in parsed):warnings.append('sample_detail_missing')
    pairs=[]
    for frame in sorted({s['frame'] for s in parsed if s['point'] in (1,2)}):
        ray=[s for s in parsed if s['frame']==frame and s['point']==1]
        shade=[s for s in parsed if s['frame']==frame and s['point']==2]
        p=dict(frame=frame,unique_pair=len(ray)==len(shade)==1)
        if p['unique_pair']:
            a,b=ray[0],shade[0]
            p.update(ray=a,shade=b,writer_serial_changed=a['serial_after']!=b['serial_before'],
                     active_writer_at_either=any(a['active']+b['active']),
                     native_scene_or_epoch_changed=(a['context'][1:11]!=b['context'][1:11]) if a['context'] and b['context'] else None,
                     resource_heap_identity_changed=(a['resource_heap'][2:6]!=b['resource_heap'][2:6]) if a['resource_heap'] and b['resource_heap'] else None)
        pairs.append(p)
    if not pairs or any(not p['unique_pair'] for p in pairs):warnings.append('phase_pair_missing_or_repeated')
    if not any(s['point']==3 for s in parsed) or not any(s['point']==8 for s in parsed):warnings.append('submit_samples_missing')
    return dict(schema='ControlTOH.SceneObservation.R427',warnings=warnings,frames=pairs,samples=parsed,
                writer_events=[r for r in events.values() if r.get('kind')==20],
                scene_lease_authorized=False,native_suppression_authorized=False,
                limit='Quiet samples do not exclude uninstrumented writers or establish future-frame ownership. Serial changes show an instrumented writer boundary, not necessarily mutation of this scene.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('jsonl');p.add_argument('--output',required=True);a=p.parse_args()
    result=analyze([json.loads(line) for line in Path(a.jsonl).read_text(encoding='utf-8-sig').splitlines() if line])
    Path(a.output).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'frames':len(result['frames']),'warnings':result['warnings'],'scene_lease_authorized':False}))
