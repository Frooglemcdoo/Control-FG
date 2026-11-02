# Earlier PT reuse audit

Reviewed 2026-11-02 against PT commit `556d6758abdf4ee45048ab8df92318e0df9d8639` and the current R294 GI source. This is a source/component checkpoint, not a new game build or a visual fix.

## Recovered work

| PT component | Existing implementation | GI decision |
| --- | --- | --- |
| `pt_geometry_snapshot.h` | Bounded range copies at BLAS recording; validates all allocations before recording; restores source states; pooled private storage; recording and flight ownership | Reuse the design after source-state admission is verified; do not import per-batch fence creation |
| `pt_owned_scene_runtime.h` | Native wrapper/token registry, range coalescing, frozen index/position streams, shader-table snapshots and submission revalidation | Reference for current owner/geometry association; do not replace current ownership coverage blindly |
| `pt_scene_capture_runtime.h` | BLAS updates and compact/clone lineage, CPU-uploaded TLAS instances, recorded-copy retention, fence progress and history retirement | Reuse contracts; native upload capture may avoid a separate instance readback, but needs current hook/identity verification |
| `pt_scene_order.h` | Queue FIFO and signal/wait graph, exact scene-version selection, unknown-write invalidation, concurrent-write rejection, bounded history compaction | Recovered unchanged into `src/gi_reuse/`; not yet connected to runtime |
| `pt_scene_queue_runtime.h` | Execute/Signal/Wait/Reset interception at the actual submitted interfaces | Reference only: it serializes native queue forwarding under a global recursive mutex |
| `pt_command_bindings.h` | Local buffer barriers plus native-wrapper state fallback | Do not import its fallback as proof of current GPU state |

## Important state gap

`FreezeGeometry` obtains the wrapper state through `pt_native_buffer_observer.h` (state at +0x20) and calls `PTCommandBindings::tracker.BufferState`. If no local barrier exists, that function normalizes nonzero native read-only defaults to COMMON. The code itself does not establish the preceding Execute batch completion or all prior-list transitions for that normalization. A queue-order graph tracks execution dependencies; it does not independently prove resource state. R294 unknown index/instance states must not simply be relabeled COMMON using this fallback.

The recovered barrier tracker also sets pending from BEGIN_ONLY and accepts other transition records without R294's matching-BEGIN/END validation. Retain the stricter current behavior until cross-list split handling is explicitly supported.

## Validation and scope

Recovered `pt_scene_order.h` and `pt_progress_sampling.h` preserve the pinned PT implementation. A follow-up splits one misleadingly indented line into three statements without changing behavior. The recovered test changes only its include paths. Run:

```sh
g++ -std=c++17 -O2 -Wall -Wextra tools/test-gi-recovered-scene-order.cpp -o /tmp/test-gi-recovered-scene-order
/tmp/test-gi-recovered-scene-order
```

Passed on this host: exact fence cuts, late signals, allocation identity, unknown writes, unordered producers, cycles, differential compaction and 20,000-update bounded history. Informational CPU timings are not game performance measurements. These tests do not validate D3D12 hooks, GPU states or visual output.

## GI journal adapter

`ordered_as_metadata.h` connects immutable copies of existing `ASEvent` journals to the recovered ordering graph. Every event requires explicit AS destination/source allocation tokens. It supports ordered list batches, replay, build/update/copy lineage and tombstones for unavailable writes. Copy/update dependencies are rechecked during selection: a later-observed unordered source mutation invalidates derived metadata too. Failed signal/wait operations invalidate the adapter. No resource state is inferred.

`test-gi-ordered-as-metadata.cpp` exercises reversed recording/submission order, replay, frozen recordings surviving reset, stale-generation rejection, same-address/different-owner rejection, cross-queue copy lineage, later competing source writes, TLAS and BLAS updates, incompatible index changes, unresolved writes, pending/failed fences and capacity. Both ordering tests are included in the Windows foundation workflow.

Local validation: both tests pass with GCC warnings treated as errors. The adapter also passes AddressSanitizer/UndefinedBehaviorSanitizer with leak detection disabled. LeakSanitizer could not run because this host denies its `/proc` task inspection; this is not a successful leak check. Windows CI for this follow-up has not yet been run.

This adapter is not a runtime replacement yet. R294 excludes AS storage from its buffer owner inventory and its AS journals carry addresses without AS allocation tokens; those addresses cannot be relabeled as identity. The host must serialize observations at actual API submission boundaries, retain required lifetimes, catch allocation exceptions and invalidate admission, and supply successful native fence outcomes. Version history is bounded and fails closed at capacity; the adapter does not yet wire the recovered graph's compaction to retained version dependencies. A copy observed before its source dependencies can be resolved remains unavailable, rather than being repaired later from guessed metadata. No new game capture is needed to establish these offline contracts.

## Native AS resource ownership follow-up

Added a separate bounded AS resource registry, populated only by successful observed committed/placed creation with initial AS state. Each admitted object receives a monotonic token; placed heaps are also retained. Existing geometry-input inventory policy is unchanged. The AS registry never evicts a known owner to admit another, rejects overlapping VA matches, and has a separate 512 MiB logical extent / 8192-object cap. Its strong references can retain native allocations during a diagnostic session; the counter is not an exact VRAM allocation measurement. Reserved/tiled storage and unobserved creation routes are not admitted.

Build/update/copy journals now record these tokens alongside their descriptors. Missing tokens remain zero and are counted, not replaced by addresses. This establishes retained resource-object identity only: placed alias activation, suballocation reuse, GPU state and bytes still require independent verification. The ordering adapter remains disconnected. `GI_AS_IDENTITY` reports coverage without a readiness claim. Added WARP tests use ordinary buffers with synthetic classification and do not execute fake AS builds.

## Integration boundary

1. Adapt current immutable command-list journals to recovered queue-order selection, preserving allocation and recording generations. Validate submission/replay/reset and failed signal/wait handling offline before enabling live hooks.
2. Audit the old native wrapper/upload hooks against the supported renderer binary and compare their state evidence with current barrier tracking. Do not copy the old state normalization without proof.
3. Reuse bounded geometry copying with a persistent retirement timeline and explicit capture activation. Retain sources for recorded-list replay and submitted GPU use. Avoid PT-off copying, descriptor mirroring, and unconditional global submission locking.
4. Request one consolidated live validation only for integration facts that existing source/tests/captures cannot establish. Do not repeat M6/M7 geometry-format or binding-discovery tests.

No runtime include, hook, copy, shader or default setting changes in this checkpoint. Replacement GI remains disabled. The earlier estimate of 2–3 capture rounds was made before this audit and is withdrawn.
