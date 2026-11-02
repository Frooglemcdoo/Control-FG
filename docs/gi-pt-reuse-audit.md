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

Recovered ordering started from R46 and was then advanced to the R103 revision described below. `pt_progress_sampling.h` retains the R46 implementation. One misleadingly indented line is split into three statements without changing behavior. The recovered test changes only its include paths. Run:

```sh
g++ -std=c++17 -O2 -Wall -Wextra tools/test-gi-recovered-scene-order.cpp -o /tmp/test-gi-recovered-scene-order
/tmp/test-gi-recovered-scene-order
```

Passed on this host: exact fence cuts, late signals, allocation identity, unknown writes, unordered producers, cycles, differential compaction and 20,000-update bounded history. Informational CPU timings are not game performance measurements. These tests do not validate D3D12 hooks, GPU states or visual output.

## GI journal adapter

`ordered_as_metadata.h` connects immutable copies of existing `ASEvent` journals to the recovered ordering graph. Every event requires explicit AS destination/source allocation tokens. It supports ordered list batches, replay, build/update/copy lineage and tombstones for unavailable writes. Copy/update dependencies are rechecked during selection: a later-observed unordered source mutation invalidates derived metadata too. Failed signal/wait operations invalidate the adapter. No resource state is inferred.

`test-gi-ordered-as-metadata.cpp` exercises reversed recording/submission order, replay, frozen recordings surviving reset, stale-generation rejection, same-address/different-owner rejection, cross-queue copy lineage, later competing source writes, TLAS and BLAS updates, incompatible index changes, unresolved writes, pending/failed fences and capacity. Both ordering tests are included in the Windows foundation workflow.

Local validation: both tests pass with GCC warnings treated as errors. The adapter also passes AddressSanitizer/UndefinedBehaviorSanitizer with leak detection disabled. LeakSanitizer could not run because this host denies its `/proc` task inspection; this is not a successful leak check. Windows foundation run `41515060529`, job `125516399613`, passed at remote commit `9e40f72dcd849f1e5bf367f74d1179ac4520ecf4`; decoded logs confirm both ordering tests, AS resource identity tests, existing runtime tests and CPU/GPU initial adapters ran successfully. WARP tests are not an in-game validation.

This adapter is not a runtime replacement yet. R294 excludes AS storage from its buffer owner inventory and its AS journals carry addresses without AS allocation tokens; those addresses cannot be relabeled as identity. The host must serialize observations at actual API submission boundaries, retain required lifetimes, catch allocation exceptions and invalidate admission, and supply successful native fence outcomes. Version history is bounded and fails closed at capacity; the adapter does not yet wire the recovered graph's compaction to retained version dependencies. A copy observed before its source dependencies can be resolved remains unavailable, rather than being repaired later from guessed metadata. No new game capture is needed to establish these offline contracts.

## Native AS resource ownership follow-up

Added a separate bounded AS resource registry, populated only by successful observed committed/placed creation with initial AS state. Each admitted object receives a monotonic token; placed heaps are also retained. Existing geometry-input inventory policy is unchanged. The AS registry never evicts a known owner to admit another, rejects overlapping VA matches, and has a separate 512 MiB logical extent / 8192-object cap. Its strong references can retain native allocations during a diagnostic session; the counter is not an exact VRAM allocation measurement. Reserved/tiled storage and unobserved creation routes are not admitted.

Build/update/copy journals now record these tokens alongside their descriptors. Missing tokens remain zero and are counted, not replaced by addresses. This establishes retained resource-object identity only: placed alias activation, suballocation reuse, GPU state and bytes still require independent verification. The ordering adapter remains disconnected. `GI_AS_IDENTITY` reports coverage without a readiness claim. Added WARP tests use ordinary buffers with synthetic classification and do not execute fake AS builds.

## Later PT R103 comparison

Also inspected `experiment/pt-glass-integration` at `2f314e8992aec822f13cafbf7efa8bb55dc3915c` (R103), beyond the earlier R46 audit. Its `pt_geometry_snapshot.h` already implements persistent retirement fences per exact queue, monotonic values, queue identity checks and failed-signal quarantine. The per-batch fence problem above describes R46, not this later revision. Use R103 as the retirement reference; do not rebuild that fix from scratch.

Advanced the recovered graph and its tests to R103: reuse cached ancestry across ordinary appends, invalidate it on signals/completion changes, and use a fixed 17-entry candidate array rather than per-selection tree allocations. The added tests cover newly appended competing writes against cached cuts, late signals, cycle recovery and 16 queue lanes. Both recovered and GI-adapter tests pass locally with warnings as errors after this update. The Windows run cited above predates this graph-only update; it validates the native AS ownership changes and the earlier graph revision.

R103 `pt_command_bindings.h::BufferState` still normalizes missing local state from native read flags to COMMON. Its queue runtime still takes a global recursive mutex around native forwarding while capture is active. The capture class still preserves BLAS copies when preservation is enabled, independently of its renderingRequested flag; this local condition does not alone establish the complete R103 startup/menu lifecycle. No claim is made that R103 retained every R46 performance symptom.

## Alternative to COPY_SOURCE transitions

Microsoft's triangle descriptor contract requires vertex, index and geometry-transform inputs to be readable in NON_PIXEL_SHADER_RESOURCE state at an actual AS build. The TLAS input contract specifies the same for instance descriptors. This suggests a compute-shader snapshot immediately at that boundary could read the inputs without moving them to COPY_SOURCE. This is an engineering inference, not proof that Control's current buffers and binding setup admit that path.

Before implementation, validate shader-readable resource flags, exact bounded raw-view alignment (including 16-bit index tails), same-list ordering, source/heap retention, pending split/alias rejection and complete native compute binding restoration. Only mod-owned output transitions would be needed. A native build's API precondition does not substitute for observing the correct boundary, and this approach must not reinterpret a later arbitrary read as safe. It may simplify source state handling; it does not remove lifetime or synchronization requirements. No shader snapshot is enabled by this checkpoint.

Primary references reviewed 2026-11-02:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_raytracing_geometry_triangles_desc
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_build_raytracing_acceleration_structure_inputs
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_flags

## Integration boundary

1. Adapt current immutable command-list journals to recovered queue-order selection, preserving allocation and recording generations. Validate submission/replay/reset and failed signal/wait handling offline before enabling live hooks.
2. Audit the old native wrapper/upload hooks against the supported renderer binary and compare their state evidence with current barrier tracking. Do not copy the old state normalization without proof.
3. Reuse bounded geometry copying with a persistent retirement timeline and explicit capture activation. Retain sources for recorded-list replay and submitted GPU use. Avoid PT-off copying, descriptor mirroring, and unconditional global submission locking.
4. Request one consolidated live validation only for integration facts that existing source/tests/captures cannot establish. Do not repeat M6/M7 geometry-format or binding-discovery tests.

No runtime include, hook, copy, shader or default setting changes in this checkpoint. Replacement GI remains disabled. The earlier estimate of 2–3 capture rounds was made before this audit and is withdrawn.

## Isolated compute snapshot prototype

Added `raw_snapshot_plan.h`, `d3d12_raw_snapshot.h` and `gi_raw_snapshot.hlsl`.
The planner rounds raw reads to complete DWORDs within the resource extent,
preserves the prefix for 16-bit indices, rejects overflow and caps each dispatch
at 65,535 groups (just below 16 MiB). The recorder rejects unsuitable resources,
missing UAV capability, same-resource reads/writes and unsupported list types
before recording. It does not change descriptor heaps or source resource state.

The existing `rt_lights::NativeBindings` is specific to an eight-slot lighting
root layout and tracks neither arbitrary root constants/SRV/UAV arguments nor a
complete general DXR pipeline binding. It is not sufficient for injection at an
arbitrary AS-build boundary. This prototype therefore remains disconnected from
native hooks. Caller-controlled manual binding restoration is exercised in an
isolated WARP fixture; that is not proof of native hook restoration.

Local exhaustive short-range tests pass with GCC warnings as errors. The Windows
workflow now compiles the snapshot shader and runs a WARP byte-copy/guard-byte
fixture with a different sentinel compute pipeline after manual restoration.
Windows foundation run `41518506065`, job `125528129798`, passed at remote
commit `6ab09cda4a7966c3d2bb3706b861a08f29ae330c`. Decoded logs confirm the
raw snapshot shader compiled, bytes/guards/manual restoration passed on WARP
with no debug-layer errors, and the existing GI foundation tests passed. The
first run exposed a Windows `max` macro conflict; the tested commit fixes it.
The local planner also passes UndefinedBehaviorSanitizer. These results do not
validate native binding hooks, real DXR geometry reads or in-game visual output.

Still required for live use: complete per-list binding observation/restoration,
placed geometry heap retention, alias and split-barrier rejection, bounded total
admission and exact-queue retirement/replay lifetime handling. The per-dispatch
cap is not a session memory budget. No in-game copying or GI setting changes.

Full Windows build/regression run `41518506100`, job `125528129227`, also passed
at the same code revision. Decoded logs confirm compilation and regression/
packaging completion without compiler errors or failed workflow steps. Its
artifact retains the existing R294 label and is not a new user-facing release.

## General compute binding restoration component

Added `compute_root_arguments.h` and `d3d12_compute_bindings.h`. The recorder
stores all root descriptor kinds and individual known constant DWORDs, preserves
bindings when the same root is set, clears arguments on root changes, and clears
tables on heap changes. Frozen snapshots retain roots, heaps and the active PSO
or raytracing state object. Invalid setter ranges/kinds or explicit invalidation
refuse capture until reset. Root setters allocate no memory.

`RawSnapshotPipeline::RecordRestoring` refuses absent binding snapshots before
recording, copies geometry, then restores the captured state without switching
heaps. The expanded WARP fixture runs a different shader afterward that consumes
a descriptor table, CBV, SRV, UAV and sparse root constants. Portable tests cover
root/heap/reset transitions, sparse constants including bit 63, invalid ranges,
and independent frozen copies. Local GCC warnings-as-errors and UBSan passed;
Windows foundation run `41520993739`, job `125536658194`, passed at remote
commit `48bf65082ecbff6a46315695c25ff4d20729b968`. Decoded logs confirm
all portable cases and the expanded GPU restore fixture passed with no
debug-layer errors. The real DXR restore branch is not exercised on WARP.

Microsoft's documented semantics were rechecked: there is one active pipeline
state across SetPipelineState/SetPipelineState1; setting a PSO does not set the
root signature; changing a root invalidates its arguments, setting the same root
does not. References:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist4-setpipelinestate1
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-a-root-signature

This is still a component, not live hook coverage. The host must associate one
tracker with each command-list recording, cover all setters and reset/clear,
invalidate unmodelled ExecuteIndirect/ExecuteBundle effects, and mute injected
calls. Restore must immediately follow capture on that same recording with no
intervening heap/native changes. WARP covers compute restoration, not actual
DXR dispatch restoration. Descriptor resource lifetimes, placed geometry heaps,
alias safety and per-queue retirement remain separate requirements.

## Geometry backing-storage leases

`d3d12_storage_lease.h` attaches observed storage kind and a strong placed-heap
interface to each successfully observed committed/placed buffer. Runtime creation
observation now sets that data before inventory admission. Barrier-only discovery
and unsupported/reserved creation routes do not manufacture storage proof.
`AcquireStorageLease` returns separate resource and heap references; missing or
malformed metadata refuses the lease. Resource private data follows resource
lifetime even across inventory eviction/readmission. This runs only with the
existing GI metadata observer enabled.

The owner budget still measures logical buffer bytes, not total retained heap
allocations. Keeping a small placed buffer may retain its larger heap; physical
heap budgeting, alias activation and GPU retirement remain admission requirements
for the eventual snapshot path. Object retention does not establish any of them.
The Windows runtime fixture now checks real intercepted committed/placed creation,
backing heap identity, independent lease retention and unknown-creation refusal.
Windows foundation run `41521688959`, job `125539041446`, passed at remote
commit `348a73d5249979c0e934265c6cd567656fc9207f`. Decoded logs confirm
the real creation-hook storage lease test, binding tests and GPU snapshot
restore fixture all passed. Full Windows build/regression run `41521688899`, job `125539041555`,
also passed at that code revision. Decoded logs show no compiler or workflow
errors. The artifact still uses the R294 label and is not a new public test build.

## Opt-in native binding observation

`gi_compute_hooks.inl` connects the tracker to real command-list setters under
both `CONTROLTOH_GI_METADATA=1` and `CONTROLTOH_GI_BINDINGS=1`. It is off by default.
Per-list COM private state survives the separate GI journal's replacement.
Observed creation establishes the initial PSO; late attachment is unknown until
successful Reset/ClearState. Failed Reset preserves the previous generation.
SetPipelineState, SetPipelineState1, compute root/table/constants/descriptors and
heap setters forward unchanged before updating observation. Common methods are
patched on both base and list4 interface tables if distinct. Coverage failure
blocks captures. Capacity is eight table classes and 4096 live tracker objects.

ExecuteIndirect and ExecuteBundle conservatively invalidate capture until a
known reset/clear. Mute excludes injected setters without blocking forwarding.
No geometry-copy call was added to native AS builds. The WARP fixture now uses
actual vtable interception for its GPU restore test; it also checks late attach,
successful reset, ClearState and bundle invalidation. Saved-forwarder stubs check
failed-reset preservation and indirect argument forwarding without submitting
invalid API calls. Actual DXR pipeline restoration still needs hardware coverage.
Windows foundation run `41526470326`, job `125555375721`, passed at remote
commit `042b8d7173ea4dcfad80a8a39026887494d42094`. Decoded logs confirm
the real setter interception, GPU binding restoration, lifecycle/invalidation
cases and command-list tracker release passed with no debug-layer errors.
Compilation first exposed a setter/vtable field collision and a WRL pointer
access error; both are fixed in the tested revision. Full Windows mod build
and regression run `41526470375`, job `125555376044`, also passed at the
same code revision. Decoded logs show no compiler or workflow errors.
The packaged artifact still bears R294 and is not a new user-facing release.

## Snapshot recording lifetime and publication policy

Re-read R103 `pt_geometry_snapshot.h::Retirement` at
`2f314e8992aec822f13cafbf7efa8bb55dc3915c`: its persistent exact-queue
fences, recording ownership through Reset and retention of unproven flights
remain the D3D12 integration reference. A completed execution alone does not
make a snapshot immutable: a closed native list can replay the same writes.

Added portable `snapshot_retirement.h` to define and test the admission,
ownership and publication rules before adapting that fence runtime. Admission
is bounded by record count and caller-supplied retained bytes and precedes GPU
command insertion. A unique owner package stays retained across every execution.
Successful signals raise the required completion value; batch entries can share
one signal. Output is available only after the recording is sealed (successful
Reset/final destruction, never Close) and all recorded writes have completed.
Unsubmitted sealed records can be freed without publishing nonexistent output.

Failed signals, missing queues, counter exhaustion, stale generations, duplicate
pending submissions and unexpected replay after sealing quarantine ownership.
Cross-queue replay is quarantined rather than certified. Completion cannot
invent unissued fence values; regression and device-removal sentinels refuse
release. Explicit DeviceStopped teardown is only valid after GPU shutdown.
The manager itself must outlive GPU use. Borrowed output views require finished
consumers before Retire; render-consumer fence leases are not implemented.
Retire detaches the owner for destruction outside host locks, avoiding COM
release reentry while a queue/observer lock is held. Detached owners remain
the host's memory-budget responsibility.

This is not a synchronization primitive and is not wired into native Execute.
Quarantining an unexpected replay cannot undo already-recorded GPU commands or
make overlapping writes safe. Live copying remains blocked on an execution/
synchronization protocol, queue/fence lifetime identities, physical heap budget
accounting and consumer retirement. Static command lists that never reset cannot
publish under this conservative policy; no latency or coverage guarantee is made.

Local GCC warnings-as-errors and UndefinedBehaviorSanitizer tests passed replay,
reset during outstanding GPU work, batched fence cuts, budget refusal, wrong
queue/generation, failed signals, counter exhaustion, device-removal sentinel,
completion regression, unknown queues and unissued completion rejection. The
Windows foundation run `41529405918`, job `125565421444`, passed at remote
commit `dca2c3a65de2c8ef3b4bc90ddfab3f549b90ba47`. Decoded logs confirm the
retirement policy, live binding-hook fixture and GPU copy/restore fixture all
passed. The retirement policy remains CPU-tested logic, not a GPU-fence runtime
integration; the GPU fixtures still exercise copying/restoration separately.

Full mod Windows build/regression run `41529405730`, job `125565421070`,
also passed at `dca2c3a65de2c8ef3b4bc90ddfab3f549b90ba47`. Decoded logs
show no compiler or workflow errors. No native snapshot dispatch was enabled;
this checkpoint has no new user-facing build or capture request.
