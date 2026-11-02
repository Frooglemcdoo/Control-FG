# Submission-ordered GI geometry metadata

`src/gi_reuse/submission_ledger.h` is a bounded, portable metadata core. It is
now reached by an opt-in native submission observer. It does not capture GPU
bytes or change lighting.

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

The opt-in runtime observer now calls the adapter from build/copy hooks, owns
per-command-list journals via COM private data, and freezes the journal at queue
submission. Full resource lifetime and GPU buffer access are still
required before claiming a live triangle mapping. No producer PDF is inferred
from this metadata, and no game lighting change is enabled.

Tests: `test-gi-geometry-metadata.cpp` covers geometry-index preservation, immutable
ownership, copy/update lineage, snapshots, stale revisions, arithmetic bounds and
unsupported inputs. `test-gi-d3d12-geometry.cpp` covers actual D3D12 array and pointer
layouts, source-array mutation, null descriptors and unsupported geometry types.

## Opt-in runtime submission wiring

`CONTROLTOH_GI_METADATA=1` enables `d3d12_submission_metadata.h`. Initialization
attaches a reference-counted journal to each instrumented command list. Successful
native resets install a new generation; destruction releases the journal. Build
and clone/compact-copy hooks record metadata before forwarding the exact native
call. Descriptor snapshots survive native descriptor-array reuse.

Execute callbacks freeze metadata before the native queue call, then enqueue a
fence afterward. Snapshot only checks the fence; it never waits. There are no
inserted command lists, split submissions or GPU buffer copies. CPU allocations
and an extra queue signal are confined to the development observer; it remains
disabled by default.

This first observer deliberately invalidates on unknown list coverage, concurrent
Execute calls, a second queue, multiple AS-bearing lists in one batch, capacity
exhaustion, unsupported geometry, serialization/deserialization, or incompatible update metadata.
Updates now require an ALLOW_UPDATE source, stable geometry order/counts/formats/flags/index addresses and preserved transform presence. TLAS updates preserve instance count. Index contents and GPU resource state remain unverified. It cannot yet establish Control's complete dynamic scene. There is no automatic
recovery that could hide missing events. A successful metadata snapshot proves
only completion of observed descriptor submissions, not ownership or contents of
vertex/index/instance buffers. No rendering path consumes it.

The WARP runtime fixture executes an empty native command list with synthetic AS
metadata, tests actual private-data ownership and queue fences, and checks reset,
missing coverage, update, concurrent-submit and unsupported-copy rejection. It
**does not execute acceleration-structure builds or validate GPU geometry bytes**.
The existing native-hook mock checks original call forwarding unchanged.

Validation on 2026-11-02: [Windows runtime and adapter run 41464800786](https://github.com/Frooglemcdoo/Control-FG/actions/runs/41464800786) passed the new WARP journal/fence fixture, native hook-forwarding tests, descriptor/ledger checks, and both RTXDI initialization shaders.

## R286 buffer inventory and dynamic updates

The native copy hooks now observe source/destination buffer objects. The inventory
retains at most 2048 objects / 512 MiB of logical buffer extents until process exit,
rejects ambiguous address resolution, and reports missing ranges. Objects retained
this way cannot be destroyed, but contents can still change; placed heaps, aliasing,
CPU writes, reserved-resource mappings and resource states are not established.
No geometry readback or rendering consumer uses these observations.

Build events carry ALLOW_UPDATE. An Update event resolves its source at submission
time, validates descriptor invariants and creates a fresh destination revision.
Copies preserve update eligibility. This validates descriptors only; unchanged
index-buffer contents remain a native API requirement, not an observed fact.
Reference checked 2026-11-02:
https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#acceleration-structure-update-constraints

Use the R286 metadata launcher for one automatic session covering startup,
traversal, updates, copies and owner coverage. GI_METADATA_R286 reports status and
an incompleteness reason without a GPU dump. No F7 or stationary view is needed.

## R287 retention correction

R286 filled all 2048 owner entries before AS builds were first logged. R287
keeps recent unused entries replaceable and protects uniquely resolved geometry
entries. Eviction is explicit telemetry and can cause a later range to be missing;
it never validates an unavailable owner. Strong references release after unlocking.
The 512 MiB logical extent and 2048-entry limits remain. All-protected exhaustion
still rejects new owners. This remains an observational development inventory,
not complete GPU resource lifetime or content tracking.

Runtime failures now distinguish journal allocation/attachment/capacity, invalid
build descriptors, unsupported BLAS/TLAS layouts, copy modes, missing update
sources, queue/fence failures and ledger contracts. R286 cannot retrospectively
identify which generic failure branch triggered.

Windows focused validation run 41473829266 passed cache churn, geometry retention,
all-protected exhaustion, specific journal-overflow reporting and existing GPU
adapter tests. Diagnostics-only collection no longer asks for an RR capture.

## R288 observation coverage and journal growth

R287 identified `build_journal_capacity`, with zero resolved and 1642 missing
ranges before detailed tracking stopped. It does not prove the individual causes
of those missing ranges. Copy-only discovery and undifferentiated upload/scene
retention were coverage gaps found in source review.

R288 adds observation at base committed/placed/reserved resource creation and
native transition/UAV/aliasing barriers. It preserves original native arguments
and does not add barriers. Newer resource-creation entry points are not separately
hooked. Default-heap scene entries outrank unreferenced upload entries for
retention; uniquely resolved geometry remains protected. Limits remain 2048
objects / 512 MiB logical extents. This is still incomplete coverage, not a claim
of placed-heap ownership, residency, aliasing synchronization or stable bytes.

Per-list journals grow on the heap to 65536 events; submission snapshots copy
owned events while sharing immutable geometry snapshots. No events are silently
truncated. Allocation/limit failures invalidate the observer. `journal_peak`,
`creation_observations`, and `barrier_observations` report which paths were reached.
Windows validation run 41477747314 passed actual committed/placed interception,
barrier discovery, scene retention under upload churn, 1024-event submission,
reset snapshot independence, and rejection at the hard journal ceiling.

Primary references checked 2026-11-02:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createcommittedresource

## R289 relevant-queue classification and input inventory

R288 progressed to a 1692-event journal with 191 resolved / 5066 missing range
lookups, then stopped at multiple_queues. The old guard ran before classifying
AS relevance; this is not evidence that AS work spans queues. R289 ignores copy
queues and known-empty instrumented journals before enforcing single-AS-queue and
concurrent-AS-submission limits. Unknown direct/compute journals remain fatal.
An unrelated submission cannot overwrite a pending AS snapshot or its token.
This does not establish cross-queue dependencies of geometry bytes.

Known AS-storage resources and readback allocations no longer occupy the input
inventory. Shader-readable/index-buffer state observations give default buffers
higher admission priority; they are hints only, not proof of AS usage or state.
Protected geometry still cannot be evicted. A failed admission leaves existing
entries intact instead of partly evicting them and then discovering insufficient
space. Limits remain 2048 objects / 512 MiB logical extents.

The first R289 runtime fixture exposed an open command-list/allocator conflict
in the test and a CI exit-code check that missed negative assertion exits.
The fixture now closes the unrelated empty list, and the validation batch rejects
all nonzero exits. The rerun 41484181379 passed, including the runtime PASS marker and no
assertion/HRESULT failures; the first job badge is not used as evidence. Tests cover unrelated submissions between AS callbacks,
genuine second-AS-queue rejection, input candidate retention, atomic admission,
and AS/readback exclusion. WARP executes empty lists, not synthetic AS builds.
Live geometry bytes and reuse remain unverified.

Primary references checked 2026-11-02:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_build_raytracing_acceleration_structure_inputs
