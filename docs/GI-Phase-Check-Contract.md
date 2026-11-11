# R425 integration evidence contract

R425 preserves R424 rendering. The opt-in check fills a specific missing join in
R424 source captures: which command-list recordings contain the exact native GI
ray state and GI6 evaluator, and how those recordings enter queue submission.
It is not a noise, performance, traversal or temporal-history fix.

`Start-GI-Phase-Check.cmd` disables source/image capture flags for its child
process, enables this bounded metadata path and collects only the matching
recent phase log. Ordinary launches do not enable it. No persistent setting is
written. Existing binding tracking is already required by NRD even with RELAX
disabled; this check reads snapshots from it, without turning on legacy scene
capture. Marker IDs survive ClearState but are cleared on successful Reset.

Limits: first 24 GI engine frames, three seconds of submission observation,
8192 preallocated records. A failed or overflowing marker is recorded explicitly.
Workers reserve independent event slots and publish with release/acquire; file
writes occur only on the existing UI timer. No GPU commands, waits or readbacks
are injected. Reported CPU events do not establish GPU completion.

## JSONL schema

Each event is `{seq, kind, data:[12 unsigned 64-bit integers]}`. Unused slots are
zero. Pointer values are opaque identities, not addresses to dereference later.
`seq` describes CPU event reservation, not an inferred cross-queue GPU order.

| Kind | Meaning | Data fields |
| --- | --- | --- |
| 1 | Exact GI ray dispatch | engine frame, list, binding generation, width, height, depth, root, state object, resource heap, sampler heap, snapshot ready, thread |
| 2 | GI6 evaluator dispatch | engine frame, list, binding generation, groups X/Y/Z, root, PSO, resource heap, sampler heap, snapshot ready, thread |
| 3 | Before ExecuteCommandLists | observed frame, queue, list count, thread, queue type |
| 4 | Submitted phase marker | execute event, list index, submitted list pointer, phase event, ordinal within recording, marker overflow |
| 5 | Queue Signal/Wait returned | observed frame, queue, fence, value, is signal, HRESULT, thread |
| 6 | Root descriptor argument | phase event, root index, RootArgumentKind, GPU address/handle |
| 7 | Root constant | phase event, root index, constant offset, value |
| 8 | ExecuteCommandLists returned | execute event, thread |
| 9 | Marker error | phase/execute event, reason (1 write, 2 invalid count) |

Status lines carry first_frame, stopped, reserved, written and dropped. The last
status must show stopped, zero dropped and reserved==written before treating the
record set as complete. A ready binding snapshot means observed tracker state;
it does NOT establish required-slot completeness or referenced-resource leases.

The parser reports same-recording order, separate lists in one Execute call,
earlier nonoverlapping Execute calls, or unresolved cross-queue/overlap cases.
It always returns native_suppression_authorized=false. Fence records alone are
not a complete cross-queue proof (CPU fence signals and overlap need validation).

## After receiving the result

1. Inspect completeness and exact ray/evaluator cardinality per engine frame.
2. Compare root identities, descriptor heaps and all captured arguments.
3. Determine whether both phase bindings can be safely replayed in one retained
   recording, or whether split scheduling must instead occur at submission.
4. Independently establish barrier states, descriptor-resource ownership and
   asynchronous native SBT writer lifetime before suppressing native work.
5. Wire disjoint receiver phases into the already tested frozen sampler. Store
   exact weights in a separate resource and apply them to all GI radiance.
6. Validate live output publication and GPU cost before adding history changes.

Do not infer moving-surface identity from current-frame guide keys, reuse the
rejected depth-only history as a new fix, or claim a visual improvement from the
metadata check. User is away; deliver the built check once all build checks pass.
