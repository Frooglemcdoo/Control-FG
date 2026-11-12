#!/usr/bin/env python3
"""Summarize existing compact logs without conflating bundled sessions or GPU timings."""
import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path


def analyze(paths):
    archives, sessions = [], {}
    for path in paths:
        archive = {"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "sessions": []}
        with zipfile.ZipFile(path) as z:
            bad = z.testzip()
            if bad:
                raise ValueError(f"CRC failure: {bad}")
            for name in z.namelist():
                if not name.endswith('.log'):
                    continue
                raw = z.read(name)
                digest = hashlib.sha256(raw).hexdigest()
                archive['sessions'].append(digest)
                if digest in sessions:
                    continue
                lines = raw.decode('utf-8', errors='strict').splitlines()
                producers = [l for l in lines if 'GI_FROZEN_R428 producer=' in l]
                consumers = [l for l in lines if 'GI_FROZEN_R428 consumer=' in l]
                def values(rows, key):
                    return [int(m.group(1)) for l in rows if (m := re.search(r'\b' + key + r'=(\d+)', l))]
                sessions[digest] = {
                    'member': name, 'sha256': digest,
                    'logged_producer_indices': values(producers, 'producer'),
                    'logged_guided_consumer_indices': values([l for l in consumers if 'guided=1 ' in l], 'consumer'),
                    'logged_complement_consumer_indices': values([l for l in consumers if 'native_complement=1' in l], 'consumer'),
                    'maximum_logged_retained_bytes': max(values(producers, 'held_bytes'), default=0),
                    'preparation_failures_logged': [l for l in lines if 'reason=preparation hr=' in l],
                    'stops': [l for l in lines if 'stopped=1' in l],
                    'fallback_details': [l for l in lines if 'fallback_detail=' in l],
                    'runtime_configuration': [l for l in lines if 'RT_MODE_R411 ' in l],
                }
        archives.append(archive)
    return {
        'archives': archives, 'unique_sessions': list(sessions.values()),
        'limits': [
            'Indices are bounded log observations, not a complete frame or invocation population.',
            'Retained bytes are aggregate owned allocations, not per-frame copy traffic or total VRAM.',
            'Preparation HRESULT 0x8007000E also covers local reservation/preflight failure; device OOM is not established.',
            'QPC events are CPU recording events, not GPU timestamps or Present cadence.',
            'Older sessions bundled again are deduplicated by content hash.',
        ],
    }


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('archives', type=Path, nargs='+')
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.write_text(json.dumps(analyze(args.archives), indent=2) + '\n')
