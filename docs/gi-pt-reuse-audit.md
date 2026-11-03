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

## R295 consolidated live integration rehearsal

Added `d3d12_snapshot_retirement.h`, adapting R103's persistent private queue
fences to the tested recording publication policy. Queue identities retain their
canonical IUnknown, queue and fence; slots are never reused. Before/After calls
are serialized by the host, without holding the lock across native Execute.
The successful private Signal follows its native submission. Intervening queue
work can delay completion but cannot make that signal complete before the
submission it covers. Poll does not block. Failed signals quarantine ownership;
unresolved state is deliberately pinned at runtime teardown. Owners are detached
before COM destruction outside the host lock. This is completion accounting,
not a cross-queue dependency graph or permission to record GPU copies.

Microsoft's ExecuteCommandLists contract clarifies that replay must wait for the
previous execution to finish; concurrent resubmission is not legal native usage.
That corrects the earlier implication that arbitrary overlapping native replays
must be supported. Future replay can still overwrite completed snapshot data,
so this conservative publication policy continues to require Reset/destruction.
Cross-queue replay remains rejected even if the application may legally order it.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists

`gi_snapshot_probe.inl` exercises this lifetime path in the real hooks using
CPU-only owner records, not rendering resources. It checks compute bindings at
AS build boundaries, follows successful Reset/private-data destruction and tracks
private queue fences. The diagnostic launcher now enables GI_BINDINGS together
with GI_METADATA. Input preflight reuses existing owner resolution to check
committed/placed/unknown storage, DENY_SHADER_RESOURCE and aligned range bounds.
One run reports these together with the existing R294 state/geometry evidence.

No native snapshot dispatch, source barriers, GPU waits, readback, lighting or
RR filtering changes are enabled. `local_preflight_pass` is not copy-ready.
Actual geometry snapshots still need alias/residency proof, physical heap budget
accounting and a GPU consumer/version protocol. Binding readiness and native
lifetime coverage must be measured in-game; WARP cannot establish Control's DXR
state-object coverage. The monitor reports refusals, batch overflow and retained
quarantined records rather than claiming complete coverage.

New WARP coverage includes an actual buffer copy/readback, legal completed replay,
a queue blocked on a test fence, command-list Reset using a different allocator
while the second execution is pending, delayed publication, copied-byte validation,
deferred owner destruction and failed-signal quarantine. The existing raw snapshot
fixture also runs the real integration probe's admission/submission/reset path.
The storage test distinguishes known committed/placed from unknown creation and
rejects an overrun. Portable lifetime/root/range tests passed locally with UBSan.

Final-code foundation validation: run `41533278708`, job `125578667886`, at
`69b6564b4f714c56d0bab7278cd333779dfde8fc` passed. Decoded logs confirm the
actual buffer-copy/readback retirement test, input preflight, binding-hook fixture
and raw snapshot/restoration fixture passed with a clean D3D12 debug layer.

Full Windows mod regression/package run `41533278568`, job `125578667525`, passed
at the same code commit. Decoded logs show no compiler/workflow errors and confirm
package paths, migration preservation, material collector and relocated restart
helper tests. Verified the downloaded artifact SHA-256, nested package checksum,
ZIP integrity, R295 binary markers, diagnostic launcher flags and install layout.
User package `ControlTOH-R295.zip`: 4,190,503 bytes, 73 members, SHA-256
`fb84a09acd8969eadd48994e254a7a3b9bf9cfdf947f096a75ef6821178747ad`.
In-game binding and lifetime coverage is now the next required input; no visual
speckle improvement is claimed by this integration test.

## R296 — exact uint16 transport at native AS boundaries

The R295 user capture contained 93,481 AS builds with bindings ready for every
build, zero missing geometry ranges and zero lifetime/fence quarantine failures.
The old raw planner rejected 17,352 of 391,339 index observations (4.434%); these
are repeated observations, not unique objects. Its aggregate counter could not
separate DWORD tail padding from the old single-axis dispatch limit.

R296 adds exact 16-bit ByteAddressBuffer loads with SM 6.2/native16 capability
checks and two-dimensional dispatch. This avoids switching descriptor heaps,
which would also invalidate graphics tables that the compute-only tracker does
not restore. Only private output is DWORD padded. The standalone planner supports
up to 256 MiB; the native diagnostic samples at most 256 KiB from each range tail.
Sources: https://github.com/microsoft/DirectXShaderCompiler/wiki/16-Bit-Scalar-Types
and https://github.com/microsoft/DirectXShaderCompiler/wiki/ByteAddressBuffer-Load-Store-Additions

The opt-in diagnostic admits at most 16 ordinary and 16 legacy-rejected samples,
with a 128 MiB monotonic allocation reservation for committed sources, private
outputs and readbacks. Existing inventory, binding references and driver costs
are separate. It rejects placed/reserved sources and conflicting local states;
unknown local state relies on the native AS input NON_PIXEL_SHADER_RESOURCE
contract. No source barriers or CPU waits are inserted. Ownership is established
before dispatch, and retained native bindings plus the injected PSO/root survive
until recording seal and private-fence completion. Future base-device queue
creation is intercepted; coverage loss and batch overflow quarantine owners.

Foundation run `41539388584`, job `125599370286`, passed at code commit
`85185fa38df40f97a0b5584d867e0ed7df494c8f`. Decoded logs confirm exact GPU
copies of six-byte sources, final two-byte elements, offset-two ranges, copies
above 16 MiB, output padding/guards and native binding restoration. The integrated
probe also passed its real copy/hash, fence/reset publication and queue-creation
hook tests with a clean D3D12 debug layer. This establishes transport fixtures;
in-game R296 evidence is still needed. No replacement lighting is enabled and no
speckle fix is claimed.

Full mod regression/package run `41539388575`, job `125599371080`, passed at
that same code commit. Decoded logs show no compiler/workflow errors and confirm
lighting shader, material collector, installation layout/migration and relocated
restart-helper checks. Downloaded artifact digest, nested checksum, ZIP integrity,
R296 binary markers, new shader sidecar and all three launcher flags were verified.
`ControlTOH-R296.zip`: 4,201,754 bytes, 74 members, SHA-256
`52965751c4fc4cba5f32a982ae5c86c769188b0b423dd96e70617d68aa20fedd`.

## R296 user capture and R297 history correction

Verified the uploaded part checksum and all three payload sizes/hashes from the
November 2 R296 diagnostics (session 50560). All 32 index samples completed:
16 ordinary and 16 legacy padding-rejected observations, no copy failures, invalid
indices or nonzero output padding. Reservation total was 21,168,128 bytes; this
is not live VRAM usage. All 14,718 rejected preflight observations were
padding-outside-source; dispatch-limit and invalid-range counts were zero.
The exact uint16 path was eligible for all 435,456 observed index ranges before
metadata fault. The 32 observations contained eight distinct offset/size/hash combinations;
these are not guaranteed unique meshes. This is not whole-scene proof.

Later the historical AS ledger hit its 8,192-address capacity at submission 3,258.
The snapshot probe correctly quarantined after that metadata fault but continued
admitting empty records, filling its 256 slots. All 32 GPU samples had already
completed before the fault. There were zero signal failures or batch overflows.

R297 uses ordered map storage with logarithmic address lookup and a 65,536-record
hard limit. It retains old copy/update sources and only removes records on an
explicit invalidation event. We do not yet have native destruction/active-membership
proof, so no age eviction is introduced. This is bounded history scaling, not
complete lifetime reclamation. Snapshot copies handle allocation failure without
publishing a partial result. The probe stops admitting after metadata fault.
GI_LEDGER_R297 reports occupancy; the legacy integration summary now reports the
actual captured-copy count rather than a hardcoded zero.

Portable tests passed with warnings-as-errors and UBSan: 12,000 distinct addresses,
copy from an early record, completion-fence gating, immutable snapshot, explicit
slot reclamation and rejection at a small configured hard limit.

Windows foundation run `41545790482`, job `125620954257`, passed at
`31ebad65a8601c4af4c931743da5b4e929c26bc8`. Decoded logs confirm the 12,000-address
ledger test, exact uint16 GPU fixtures and integrated native index sampler passed
with a clean D3D12 debug layer.

Full mod run `41545790591`, job `125620954841`, passed at the same code commit.
Decoded logs show no compiler/workflow errors; package layout, migration and
restart-helper checks passed. Artifact digest, nested checksum, ZIP integrity,
R297 ledger marker, R296 sampler markers, sidecar and launcher flags verified.
`ControlTOH-R297.zip`: 4,201,127 bytes, 74 members, SHA-256
`3d0b0b337cb25b58661bcfc94d068c5622e576e78deaead76511926f6e70332a`.
Next evidence: two-minute traversal via the metadata launcher, examining occupancy
and continued tracking past R296's failure point. No replacement GI is enabled.

## R297 capture and R298 parser evidence

Verified the November 2 session 44192 uploaded archive checksum and all payload
sizes/hashes. All 32 GPU samples completed without errors. Metadata stopped with
blas_unsupported_descriptor at 5,560 historical addresses; therefore this run
did not exercise the old 8,192 ceiling. Last good heartbeat: builds=62,597,
submission=2,462; first fault heartbeat: builds=69,186, submission=2,602. No missing
geometry ranges, signal failures, batch overflows or admission refusals were
reported. Seventeen remaining owners quarantined after fault as designed.
The old generic rejection does not identify the actual descriptor field.

R298 supports aligned float3 strides of at least 12 bytes while retaining the
uint16 indexed decoder contract. Vertex extent now excludes unused trailing
padding, using overflow-checked (count-1)*stride+12 consistently in ownership
checks and scene plans. This is supported metadata, not a claim that a later
packed-layout decoder accepts every stride. Microsoft documents component-size
alignment for stride in D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_raytracing_geometry_triangles_desc

Structured first-rejection evidence records geometry index/type, build count,
layout and flags, addresses, formats, counts and stride. The CPU descriptor copy
is made while native pointers are valid and does not retain those pointers.
Publication still stops on an unsupported build; native execution is untouched.
No assertion is made that stride caused the R297 rejection. Other formats, empty
geometry and procedural geometry remain outside the supported decoder contract.
Portable geometry and ledger tests passed with warnings-as-errors and UBSan.

Windows foundation run `41560113235`, job `125668461861`, passed at
`89e922db21dc7294594514a2fe0325babdb34a79`. Decoded logs confirm padded-stride
extent checks, pointer-array parsing, retained structured rejection details and
the native index-copy/readback fixture passed. The initial report-variable naming
collision was corrected before this validated commit.

Full Windows run `41560113293`, job `125668462180`, passed at that code commit.
Decoded logs show no compiler/workflow errors; lighting switching, package layout,
migration and restart-helper checks passed. Verified artifact digest, nested
checksum, ZIP integrity, new rejection marker, existing index/ledger markers,
shader sidecar and launcher flags. `ControlTOH-R298.zip`: 4,203,167 bytes,
74 members, SHA-256
`9cb0a2e61545feccb624476a07861796113eaf303de5039593fa9df9e400a950`.
Next evidence is a two-minute automatic metadata run. No visual improvement or
resolution of the unknown R297 descriptor rejection is claimed before that run.

## R298 capture and R299 uint32 index support

The November 3 R298 capture rejected geometry 0 of a three-geometry BLAS:
vertex VA 163708010496, index VA 161909921216, stride 12, vertex count 91,799,
index count 28,086, vertex format 6, index format 42, geometry flags 1.
Format 42 is DXGI_FORMAT_R32_UINT. This is an index-width support gap, not the
padded vertex-stride case. Ledger occupancy was 6,070; all 32 prior index samples
completed with zero index-bound or padding failures. No R298 recapture is needed.

R299 handles uint16 and uint32 in metadata validation, ownership ranges, scene
read plans and primitive offsets. The offline decoder also accepts aligned
padded float3 strides. IndexSample validates width/alignment before dispatch and
decodes completed readback by that width. The existing shader preserves all bits
by combining two native uint16 loads into each DWORD; it does not narrow indices.
A third quota permits 16 uint32 samples, independent of the two uint16 categories;
the overall reservation cap remains 128 MiB. Logs identify index byte width.

Portable tests passed: captured descriptor, 12-byte primitive offset, alignment
rejection, uint32 indices over 65,535, padded vertices, truncated buffers, plus
ledger, queue order, buffer tracking, snapshot lifetime and copy-plan regressions.
Windows foundation run 41625681723, job 125882300927 passed at
`e5bdede478e6455e7b7fc524712e780974f7a56b`. Decoded logs confirm both native sampler
GPU runs passed. The --32 fixture asserts a preserved value above 65,535, exact
hash, valid bounds and padding, with native binding restoration and clean debug
layer. No native lighting change or speckle correction is enabled.

Full Windows run 41625681497, job 125882299825 passed at the same commit.
Decoded logs confirm material collection, 17-shader package layout, migration,
restart helper and lighting regressions passed. Artifact 13226540948 is
ControlTOH-R299 (4,847,856 bytes), reported digest
c476a24dc7450c29f9f621cd7d49d6e0c0e5ab838f348f57d22e89a323fac88f.
Two download attempts failed with transfer-storage ServerBusy/account ingress
limit. Therefore no local artifact digest or nested ZIP verification is claimed.
The build artifact remains available through GitHub Actions. R299 package contents
were verified by CI; local download and Library delivery remain pending.
