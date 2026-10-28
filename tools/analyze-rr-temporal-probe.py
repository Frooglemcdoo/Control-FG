"""Analyze R192 authenticated CPU-provider logs; never infer GPU consumption."""
import argparse
import json
import re
from pathlib import Path

FIELDS = re.compile(r'(\w+)=([^\s]+)')

def analyze(text):
    segments, active = [], []
    rows = invalid = cuts = 0
    previous = None
    def flush():
        nonlocal active
        if active: segments.append(active)
        active = []
    for line in text.splitlines():
        if not re.search(r'rr_temporal_probe build=R19[23] ', line): continue
        rows += 1
        fields = dict(FIELDS.findall(line))
        try:
            row = {k: int(fields[k]) for k in ('frame','samples','attempt','valid','temporal','camera_cut')}
            assert 0 <= row['temporal'] <= 0xffffffff
            assert row['valid'] == 1 and fields['cpu_provider'] == '1' and fields['gpu_confirmed'] == '0'
            assert fields['stage'] == 'pre_reflection_filter' and fields['reason'] == 'ready'
            assert row['samples'] == 4 and row['camera_cut'] in (0,1)
            if fields.get('build') == 'R193':
                row['current'] = int(fields['current'])
                assert 0 <= row['current'] <= 0xffffffff
        except (KeyError, ValueError, AssertionError):
            invalid += 1; flush(); previous = None; continue
        if row['camera_cut']:
            cuts += 1; flush(); previous = None; continue
        if previous is not None and (row['frame'] != previous['frame']+1 or row['attempt'] != previous['attempt']+1):
            flush()
        active.append(row); previous = row
    flush()
    reports = []
    for segment in segments:
        values = [r['temporal'] for r in segment]
        n = len(values)
        period = next((p for p in range(1,min(64,n//3)+1) if all(values[i]==values[i-p] for i in range(p,n))),None)
        increments = [((b-a)&0xffffffff) for a,b in zip(values,values[1:])]
        currents=[r['current'] for r in segment if 'current' in r]
        reports.append(dict(current_values_available=len(currents)==n,
                            current_increments_by_one=n>=256 and len(currents)==n and all(((b-a)&0xffffffff)==1 for a,b in zip(currents,currents[1:])),
                            frames=n,first_frame=segment[0]['frame'],last_frame=segment[-1]['frame'],
                            distinct_values=len(set(values)),minimum=min(values),maximum=max(values),
                            sufficient_window=n>=256,observed_period_up_to_64=period if n>=256 else None,
                            all_increment_by_one=n>=256 and all(d==1 for d in increments),
                            repeated_adjacent_values=sum(d==0 for d in increments)))
    return dict(cpu_provider_only=True,gpu_consumption_verified=False,rows=rows,invalid_rows=invalid,
                camera_cut_rows=cuts,segments=reports,
                sufficient_window=any(s['sufficient_window'] for s in reports),
                note='Period is observed within this window, not proof of perpetual behavior. Gaps, cuts and invalid reads split windows.')

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path)
    args=parser.parse_args()
    print(json.dumps(analyze(args.log.read_text(errors='replace')),indent=2))
