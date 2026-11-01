# Submission-ordered GI geometry metadata

`src/gi_reuse/submission_ledger.h` is a bounded, portable metadata core. It is
not hooked into Control yet and does not capture GPU bytes or change rendering.

CPU recording order cannot establish the AS state at a GI dispatch. The adapter
must collect a journal for each command-list generation, then process journals
in actual queue submission order. The core tracks build replacement, AS copy
lineage and explicit invalidation. Reused addresses receive new revisions.
Copies preserve source metadata as it existed at that event, not a mutable alias.
Overflow, stale generation, missing copy source, unknown operation and a second
queue invalidate the claim rather than silently returning incomplete metadata.

A snapshot requires the correct queue and a completed fence covering the last
submission. UINT64_MAX is rejected because it indicates device removal. The
returned snapshot is a value copy and is unaffected by subsequent updates.
`Find` is a metadata lookup, not proof of completion; use `Snapshot` before
exporting a completion-qualified result. The caller must synchronize access.

This deliberately restricted core does not prove that barriers, resource states,
resource lifetimes, or inter-queue dependencies are correct. It tracks a single
ordered stream only. A native adapter must establish submission ordering under
concurrent ExecuteCommandLists calls, track all relevant mutations and ownership,
and preserve same-generation bytes. Multiple lists in one ExecuteCommandLists
call may overlap; metadata order alone does not establish a GPU memory dependency.
Do not split native submissions or insert blocking waits to fit this model.

A Build event represents a fully resolved descriptor snapshot, including an
in-place update's resulting metadata. ASRecord currently carries the AS identity,
revision and TLAS instance-buffer metadata; it is not yet a full BLAS triangle
registry. Opaque or unsupported copy/update operations must invalidate tracking.
The adapter and GPU snapshot remain required before this can decode live GI hits.

Validation: `g++ -std=c++17 -Wall -Wextra -Werror
 tools/test-gi-submission-ledger.cpp -o test-ledger` then run test-ledger.
Tests cover reversed recording/submission order, stale resets, copy preservation,
address reuse, immutable completed snapshots, fence gating and rejection paths.
AddressSanitizer/UndefinedBehaviorSanitizer also pass with leak detection disabled;
LeakSanitizer cannot inspect process threads in this execution environment.

Primary references, checked 2026-11-01:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-getcompletedvalue
