"""M7 historical address candidates only. Never generates renderer bindings."""
import argparse
import collections
import json


def analyze(events):
    # Union of complete, preceding CPU build observations. Keeping old candidates
    # deliberately exposes address reuse as ambiguity instead of guessing lifetime.
    geometry = collections.defaultdict(set)
    pending = {}
    counts = collections.Counter()
    complete = False
    write_ok = False
    dropped = 0
    tables = {}
    for e in events:
        kind = e.get('kind')
        seq = e.get('sequence')
        if kind == 1:
            pending[seq] = (e['count'], {})
        elif kind == 4:
            pending.pop(seq, None)
            counts['geometry_read_errors'] += 1
        elif kind == 3 and seq in pending:
            n, rows = pending[seq]
            rows[e['ordinal']] = e
            if len(rows) == n and set(rows) == set(range(n)):
                for g in rows.values():
                    stride = {57: 2, 42: 4}.get(g['index_format'])
                    if g['geometry_type'] != 0 or not stride or g['vertex_format'] != 6 or g['transform'] != '0':
                        continue
                    key = (int(g['index'], 16), stride)
                    geometry[key].add((g['position'], g['stride'], g['index_count'], g['vertex_count']))
                del pending[seq]
        elif kind == 5:
            tables[seq] = {'expected': e['vertex_count'], 'seen': set()}
            counts['tables'] += 1
            counts['omitted_binding_rows'] += max(0, e['count'] - e['vertex_count'])
        elif kind == 7:
            counts['binding_read_errors'] += 1
        elif kind == 6:
            counts['binding_rows'] += 1
            if seq in tables:
                tables[seq]['seen'].add(e['ordinal'])
            c = e['constants']
            base = int(e['index'], 16)
            address = base + c[0]
            if not base or c[1] not in (2, 4) or address >= 2**64 or address % c[1] or c[0] >= c[8]:
                counts['invalid_index_binding'] += 1
                continue
            candidates = geometry.get((address, c[1]), set())
            # Bounds only reject impossible candidates; they cannot prove that
            # a resource is retained, current, or referenced by this table.
            candidates = {g for g in candidates if g[2] * c[1] <= c[8] - c[0]}
            counts['unique_candidate' if len(candidates) == 1 else 'ambiguous_candidate' if candidates else 'missing_candidate'] += 1
        elif kind == 'end':
            complete = True
            write_ok = bool(e['write_ok'])
            dropped = e['dropped']
    return dict(counts, complete=complete, write_ok=write_ok, dropped=dropped,
                incomplete_builds=len(pending),
                incomplete_tables=sum(t['seen'] != set(range(t['expected'])) for t in tables.values()),
                lifetime_verified=False, renderer_ready=False,
                scope='Historical preceding CPU observations; address matches are candidates, not proven correspondence or lifetime.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('jsonl')
    args = parser.parse_args()
    with open(args.jsonl, encoding='utf-8-sig') as f:
        print(json.dumps(analyze(json.loads(line) for line in f if line.strip()), indent=2))
