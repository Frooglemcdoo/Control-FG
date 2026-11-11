"""Describe bounded R425 CPU metadata. Never authorizes native-work suppression."""
import json
import sys
from collections import defaultdict
from pathlib import Path


def analyze(rows):
    events = {r['seq']: r for r in rows if 'seq' in r}
    status = next((r for r in reversed(rows) if r.get('status')), {})
    warnings = []
    if sorted(events) != list(range(1, len(events) + 1)):
        warnings.append('missing_or_duplicate_event_sequence')
    if len(events) != sum('seq' in r for r in rows):
        warnings.append('duplicate_event_id')
    if not status.get('stopped'):
        warnings.append('observation_window_not_finished')
    if status.get('dropped', 0) or status.get('reserved') != status.get('written'):
        warnings.append('incomplete_or_overflowed_trace')
    phases = {i: r for i, r in events.items() if r['kind'] in (1, 2)}
    calls = {i: r for i, r in events.items() if r['kind'] == 3}
    ends = {r['data'][0]: i for i, r in events.items() if r['kind'] == 8}
    submissions = defaultdict(list)
    for r in events.values():
        if r['kind'] == 9:
            warnings.append('marker_write_or_read_failed')
        if r['kind'] != 4:
            continue
        call, index, pointer, phase, ordinal, overflow = r['data'][:6]
        if overflow:
            warnings.append('recording_marker_overflow')
        if phase not in phases or call not in calls:
            warnings.append('unresolved_recording_or_submission_reference')
            continue
        submissions[phase].append((call, index, ordinal))
    grouped = defaultdict(lambda: {1: [], 2: []})
    for ident, row in phases.items():
        grouped[row['data'][0]][row['kind']].append(ident)
        if not row['data'][10]:
            warnings.append('binding_snapshot_unavailable')
        if len(submissions[ident]) != 1:
            warnings.append('phase_submission_missing_or_repeated')
    descriptors = defaultdict(dict)
    for row in events.values():
        if row['kind'] == 6:
            ident, index, kind, value = row['data'][:4]
            if index in descriptors[ident]:
                warnings.append('duplicate_binding_slot')
            descriptors[ident][index] = (kind, value)
    pairs = []
    target_end = 0
    target_complete = bool(phases)

    for frame, kinds in sorted(grouped.items()):
        if len(kinds[1]) != 1 or len(kinds[2]) != 1:
            target_complete = False
            pairs.append(dict(frame=frame, relationship='ambiguous_phase_count', rays=kinds[1], evaluators=kinds[2]))
            continue
        ray, shade = kinds[1][0], kinds[2][0]
        relation = 'recording_only'
        if len(submissions[ray]) == len(submissions[shade]) == 1:
            rc, ri, ro = submissions[ray][0]
            sc, si, so = submissions[shade][0]
            rq, sq = calls[rc]['data'][1], calls[sc]['data'][1]
            if rc not in ends or sc not in ends:
                target_complete = False
            else:
                target_end = max(target_end, ends[rc], ends[sc])
            if rc == sc and ri == si:
                relation = 'same_recording_ray_before_evaluator' if ro < so else 'same_recording_reversed'
            elif rq != sq:
                relation = 'different_queues_requires_dependency_analysis'
            elif rc == sc:
                relation = 'same_execute_call_distinct_lists'
            elif rc in ends and ends[rc] < sc:
                relation = 'ray_in_earlier_nonoverlapping_execute_call'
            else:
                relation = 'overlapping_or_reversed_execute_calls'
        else:
            target_complete = False
        rb, sb = descriptors[ray], descriptors[shade]
        if set(rb) != set(range(8)) or set(sb) != set(range(8)):
            target_complete = False
        pairs.append(dict(frame=frame, ray_event=ray, evaluator_event=shade, relationship=relation,
                          differing_binding_slots=[i for i in sorted(set(rb) | set(sb)) if rb.get(i) != sb.get(i)],
                          same_root=phases[ray]['data'][6] == phases[shade]['data'][6],
                          same_heaps=phases[ray]['data'][8:10] == phases[shade]['data'][8:10]))
    if not phases:
        warnings.append('no_gi_phases')
    frames = sorted(grouped)
    target_complete = (target_complete and frames == list(range(status.get('first_frame', -1), status.get('first_frame', -1)+24))
                       and all(i in events for i in range(1, target_end+1))
                       and not any(w != 'incomplete_or_overflowed_trace' for w in warnings))
    return dict(phase_records=len(phases),
                target_prefix=dict(recording_and_submission_complete=target_complete,
                                   last_required_event=target_end,
                                   scope='Only the recorded 24-frame phase window; later overflow remains a trace-wide warning, not GPU/resource admission'), frame_relationships=pairs, warnings=sorted(set(warnings)),
                native_suppression_authorized=False,
                limitations='CPU metadata only; no GPU completion, barrier/resource ownership, SBT writer lifetime or temporal correctness proof')


if __name__ == '__main__':
    rows = [json.loads(line) for line in Path(sys.argv[1]).read_text(encoding='utf-8-sig').splitlines() if line.strip()]
    print(json.dumps(analyze(rows), indent=2))
