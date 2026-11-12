#!/usr/bin/env python3
"""Recompute saved R414 frame and GPU means; retain its incomplete-run status."""
import argparse
import collections
import csv
import hashlib
import io
import json
import statistics
import zipfile
from pathlib import Path


def analyze(path):
    with zipfile.ZipFile(path) as z:
        if z.testzip():
            raise ValueError('Archive CRC failure')
        names = {n.removeprefix('./'): n for n in z.namelist()}
        def read(name):
            return z.read(names[name]).decode('utf-8-sig')
        def rows(name):
            return list(csv.DictReader(io.StringIO(read(name))))
        frames = collections.defaultdict(list)
        for r in rows('frame-times.csv'):
            frames[r['phase']].append(float(r['interval_ms']))
        stages = collections.defaultdict(lambda: collections.defaultdict(float))
        for r in rows('gpu-timings.csv'):
            stages[r['phase'], r['stage']][r['present']] += float(r['gpu_ms'])
        phases = []
        for r in rows('comparison.csv'):
            values = frames[r['phase']]
            mean = statistics.mean(values) if values else 0
            assert len(values) == int(r['intervals']), r
            assert abs(mean - float(r['mean_frame_ms'])) < 0.0001, r
            phases.append({'phase': r['phase'], 'intervals': len(values), 'mean_frame_ms': mean,
                           'coverage_ok': r['coverage_ok'] == '1',
                           'skipped_gpu_samples': int(r['skipped_gpu_samples'])})
        gpu = []
        for r in rows('gpu-stage-summary.csv'):
            values = list(stages[r['phase'], r['stage']].values())
            mean = statistics.mean(values)
            assert len(values) == int(r['sampled_presents']), r
            assert abs(mean - float(r['mean_gpu_ms_per_sampled_present'])) < 0.0001, r
            gpu.append({'phase': r['phase'], 'stage': r['stage'], 'sampled_presents': len(values), 'mean_gpu_ms': mean})
        summary = read('SUMMARY.txt')
        return {'file': path.name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                'outcome': next(l for l in summary.splitlines() if l.startswith('Outcome:')),
                'verified_frame_rows': sum(map(len, frames.values())),
                'phases': phases, 'gpu_stages': gpu, 'relax_failure_rows': len(rows('relax-failures.csv')),
                'members': {n: hashlib.sha256(z.read(raw)).hexdigest() for n, raw in names.items() if not n.endswith('/')},
                'limits': ['Global comparison is incomplete; completed phases remain partial historical evidence.',
                           'RR return lacks coverage and no-query/profile-return controls have no frames.',
                           'Stage durations are selected GPU spans, not recoverable frame-time savings.',
                           'This predates the frozen path and does not time its copies, preparation gate or split scheduling.',
                           'Loaded SR is not an untouched-engine reference; glass was disabled.']}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('archive', type=Path)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    report = analyze(a.archive)
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f"Verified {report['verified_frame_rows']} frame rows and {len(report['gpu_stages'])} stage aggregates; {report['outcome']}")
