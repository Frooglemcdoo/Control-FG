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
in-place update's resulting metadata. ASRecord now also carries an immutable BLAS geometry snapshot when supplied.
Copies preserve that snapshot; builds replace it and assign a new AS revision. Opaque or unsupported copy/update operations must invalidate tracking.
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

## Native descriptor adapter

`geometry_metadata.h` validates and owns copied triangle descriptors.
`d3d12_geometry_metadata.h` converts both D3D12 descriptor-array layouts without
retaining pointers into the application's temporary arrays. The supported subset
is float3 positions with 12-byte stride and uint16 indices, matching R279's
observed builds. Unknown formats, procedural geometry, malformed ranges and
unsupported flags reject the entire snapshot; geometry indices are never compacted.
Geometry transform addresses are retained but their matrices are not yet read.

`PlanTriangle` requires a matching AS revision and returns a bounded six-byte
index-read plan. It does not decode indices, dereference addresses, or establish
GPU completion. Use completion-qualified snapshots and separately retained
resources/same-generation bytes before acting on the plan. Stale plans must not
be used after resource destruction or aliasing. AS metadata ownership does not
retain ID3D12Resource objects or their contents.

The existing runtime probe still only inventories descriptors. The new adapter
is not yet called from its hooks: command-list journals, queue submission ordering,
resource lifetime, compaction/update events and GPU buffer access must be wired
together before claiming a live triangle mapping. No producer PDF is inferred
from this metadata, and no game lighting change is enabled.

Tests: `test-gi-geometry-metadata.cpp` covers geometry-index preservation, immutable
ownership, copy/update lineage, snapshots, stale revisions, arithmetic bounds and
unsupported inputs. `test-gi-d3d12-geometry.cpp` covers actual D3D12 array and pointer
layouts, source-array mutation, null descriptors and unsupported geometry types.
