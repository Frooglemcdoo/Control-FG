"""Analyze cumulative storage timings without treating lifetime means as frame costs.

Accept .log files or compact-log ZIPs (latest probe session only). Output JSON.
Interval totals are reconstructed from rounded means and therefore approximate.
First observations/reset counters are baselines, never measured intervals.
"""
import argparse
import json
import re
import zipfile
from pathlib import Path

FIELDS = re.compile(r'(\w+)=([^\s]+)')


def analyze(text):
    previous, final, intervals = {}, {}, []
    present = None
    for line in text.splitlines():
        if re.search(r'DIRECTSTORAGE_R\d+_STATUS\b', line):
            present = int(dict(FIELDS.findall(line))['present'])
        if not re.search(r'DIRECTSTORAGE_R\d+_LATENCY\b', line):
            continue
        fields = dict(FIELDS.findall(line))
        if fields.get('level') != '-1':
            continue
        stage = fields['stage']
        current = {key: int(fields[key]) for key in ('samples', 'ge16ms', 'ge100ms')}
        current['mean_us'] = float(fields['mean_us'])
        current['max_us'] = int(fields['max_us'])
        current['qpc'] = int(fields['qpc'])
        old = previous.get(stage)
        previous[stage] = current
        final[stage] = current
        if old is None or current['qpc'] <= old['qpc']:
            continue
        count = current['samples'] - old['samples']
        if count <= 0 or any(current[k] < old[k] for k in ('ge16ms', 'ge100ms')):
            continue
        total = current['samples'] * current['mean_us'] - old['samples'] * old['mean_us']
        # Logs round cumulative means to 0.1us. Error increases for small deltas.
        uncertainty = 0.05 * (current['samples'] + old['samples']) / count
        mean = total / count
        intervals.append(dict(stage=stage, present=present, samples=count,
                              mean_ms=mean / 1000, rounding_bound_ms=uncertainty / 1000,
                              inconsistent=mean < -uncertainty,
                              ge16ms=current['ge16ms']-old['ge16ms'],
                              ge100ms=current['ge100ms']-old['ge100ms']))
    return dict(final_cumulative=final, intervals=intervals,
                caveats=['First retained snapshot is a baseline; earlier events cannot be localized.',
                         'Intervals are report windows, not frames or identified gameplay phases.',
                         'Means are approximate; counters are concurrent, not atomic snapshots.',
                         'Request and batch metrics have different populations and must not be added.',
                         'Native handoff precedes actual renderer publication and visible use.'])


def read_session(path):
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            names = [n for n in archive.namelist() if re.search(r'probe-\d{8}-\d{6}-\d{3}-\d+\.log$', n)]
            if not names:
                raise ValueError(f'No probe sessions in {path}')
            name = max(names, key=lambda n: n.replace('\\', '/').rsplit('/', 1)[-1])
            return name, archive.read(name).decode('utf-8', errors='replace')
    return path.name, path.read_text(encoding='utf-8', errors='replace')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('paths', nargs='+', type=Path)
    args = parser.parse_args()
    output = []
    for path in args.paths:
        session, content = read_session(path)
        output.append(dict(input=str(path), session=session, **analyze(content)))
    print(json.dumps(output, indent=2))
