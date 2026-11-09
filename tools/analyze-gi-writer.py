#!/usr/bin/env python3
"""Analyze R305 CPU writer exports; does not infer GPU ownership or noise causes."""
import argparse
import collections
import importlib.util
import json
from pathlib import Path
import struct


def analyze(folder, pid):
    spec = importlib.util.spec_from_file_location('collector', Path(__file__).with_name('collect-rr-boundary.py'))
    collector = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(collector)
    validation = collector.validate_writer_files(folder, pid)
    if validation['errors'] or not validation['complete_tables']:
        raise ValueError(validation)
    snapshots, mappings = [], []
    for meta in sorted(folder.glob(f'GI-Writer-R305-{pid}-*.json'), key=lambda f: json.loads(f.read_text())['serial']):
        m = json.loads(meta.read_text())
        rows = list(struct.iter_unpack('<IIQQ10I', meta.with_name(meta.stem+'-bindings.bin').read_bytes()))
        mapping = collections.defaultdict(set)
        for r in rows:
            # Export type, index/attribute roots, all layout/bounds except material.
            # This deliberately does not claim BLAS/position/allocation identity.
            mapping[r[1:7]+r[8:]].add(r[7])
        snapshots.append(dict(serial=m['serial'], frame=m['engine_frame'], width=m['width'], height=m['height'],
                              bindings=len(rows), opaque=sum(r[1] == 0 for r in rows), alpha=sum(r[1] == 1 for r in rows),
                              materials=len({r[7] for r in rows}),
                              index_strides=dict(collections.Counter(r[5] for r in rows)),
                              attribute_strides=dict(collections.Counter(r[6] for r in rows)),
                              zero_roots=sum(not r[2] or not r[3] for r in rows),
                              zero_bounds=sum(not r[12] or not r[13] for r in rows),
                              index_offset_outside=sum(r[4] >= r[12] for r in rows),
                              sentinel_material=sum(r[7] == 65535 for r in rows),
                              root_layout_keys_with_multiple_materials=sum(len(v) > 1 for v in mapping.values())))
        mappings.append(mapping)
    comparisons = []
    for i in range(len(mappings)-1):
        a, b = mappings[i:i+2]
        shared = [k for k in a.keys() & b.keys() if len(a[k]) == len(b[k]) == 1]
        comparisons.append(dict(first=snapshots[i]['serial'], second=snapshots[i+1]['serial'],
                                common_single_material_root_layout_keys=len(shared),
                                changed_material=sum(a[k] != b[k] for k in shared)))
    return dict(schema='gi-writer-analysis-r305', pid=pid, validation_errors=validation['errors'],
                total_binding_observations=sum(s['bindings'] for s in snapshots),
                total_alpha_observations=sum(s['alpha'] for s in snapshots), snapshots=snapshots,
                cross_snapshot_address_comparisons=comparisons,
                limits=['CPU input reconstruction, not a GPU SBT readback.',
                        'Cross-snapshot addresses do not prove allocation identity; address reuse is not excluded.',
                        'No mapping of a particular material ID to the pictured plant or hair.',
                        'No root cause for speckles established by table validity.'])


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('folder', type=Path)
    p.add_argument('--pid', type=int, required=True)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    result = analyze(args.folder, args.pid)
    if args.output:
        args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
