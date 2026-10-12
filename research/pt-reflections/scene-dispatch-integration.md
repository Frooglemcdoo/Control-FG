# Scene dispatch integration checkpoint

M11 remains on hold. Do not request another CPU-only game capture to validate
this change. The next requested user run should exercise visible integrated
rendering, or be preceded by an explicit explanation of an unavoidable blocker.

## Implemented

`pt_scene_dispatch_plan.h` combines a selected TLAS snapshot, exact BLAS geometry,
native table snapshot, allocation generations and shader identifier mapping into
one complete private hit table. Selection checks cover TLAS/table addresses,
their versions and native/private state object identity. It never looks up a
global latest scene and publishes no partial output. Both native ray types keep
their record numbers. Unreferenced rows currently reject the entire transaction.

Index/position tokens are frozen with BLAS metadata; index/attribute tokens are
frozen with native bindings. Identical GPU addresses and even a later identical
geometry observation cannot revive an old allocation identity. The resulting
plan lists the geometry allocation tokens that a runtime host must retain.

Private R16 records allow the exact DWORD round-up of the native logical index
bound only when the retained backing allocation covers it. Native attribute
views may exceed the selected geometry's used vertex range, but must still fit
the retained backing view. Neither case admits arbitrary enlarged bounds.

The existing DXR geometry fixture now feeds its actual BLAS/TLAS inputs through
this combined path before uploading and dispatching the private table. It also
corrects the old first-geometry fixture, whose BLAS used the R32 buffer while its
hit record used a separate R16 buffer containing equivalent values. Source
identifiers in this fixture remain synthetic; this is not a Control frame replay.

## Remaining runtime boundary

This is CPU preflight plus a standalone DXR fixture, not a playable Control PT
implementation. The M11 observer is still diagnostic and its aggregate relation
checks still use a latest CPU snapshot. It does not invoke the new planner.

The live host must select snapshots at the actual native dispatch, retain the
TLAS/BLAS and geometry/table/descriptor resources, prove GPU ordering and resource
states, and retire them after the actual queue fence. COM retention alone does
not freeze mutable buffer contents. It must also create/select the private state
object and bind its matching global/local roots without disturbing native state.

After that, primary PT radiance still needs the material-composition integration
described in entrance-composition-contract.md. Writing fully weighted radiance
into the native diffuse coefficient buffer would apply albedo twice. A scene
adapter or passing fixture must not be described as completing that renderer.

## Validation

Local C++ tests cover complete conversion, wrong dispatch selections, foreign
pipeline mapping, snapshot mutation isolation, conflicting instance routing,
late-record failure, reused allocations and halfword-tail padding. Existing
admission/converter/topology tests and the confirmed runtime hash gate also pass.
Windows build and DXR execution status must be checked separately; a no-DXR skip
is never a GPU pass.

The first combined path was validated at remote commit
`3c891ab045fc81a5e2b4f11aeaf11716306b59b5`: all six Windows workflows passed.
Run 38308605681 executed the geometry cases on Microsoft Basic Render Driver,
including translated and mirrored/nonuniform-scaled instances. This was reference
driver execution, not RTX hardware or Control integration validation.

## Native writer ordering and the next connection

Inspection of the existing hash-locked Steam d3d DLL establishes:

| RVA | Native operation |
| --- | --- |
| 354A2 / 354A5 | Load the state object's first pointer and SetPipelineState1 |
| 357C6 | Optional inline table writer |
| 358E4 | Record DispatchRays |
| 359D9 | runJob through import 5D650, after dispatch recording |
| 35A14 | Ray queue Wait (slot 15) for the table-copy fence |
| 374A1 | Worker calls the same table writer 33A40 |
| 313FB | waitJob through import 5D648 |
| 314C9 | Copy queue Signal (slot 14), after the job join and copy submission |

Thus the SBT is not necessarily CPU-ready at DispatchRays, or even at its queue
submission. A latest-ready-table lookup would select the wrong generation. The
live connection needs a dispatch reservation whose worker publishes before its
job returns, with the reservation linked to that exact job and buffer generation.
The native wait protects the native copy work; it must not be assumed to protect
arbitrary extra GPU work without matching its queue position and resource states.

`pt_native_shader_table.h` now reconstructs the 96-byte native hit records from
CPU writer inputs. State+0 identifies the PSO; state+20 holds four identifier
pointers indexed by `2 * geometryType + rayType`. The next 56 bytes are copied
from binding+8. Only the recovered two-ray layout is admitted. The one-ray writer
still reserves a second row, so treating it as a compact one-ray table is wrong.
No native upload buffer mapping or GPU readback is needed for this decoder.

The DXR fixture now reserves a table, submits behind a ready fence, then has a
worker construct and upload the private records. Valid test rays are published
last; failure leaves invalid rays that cannot execute TraceRay. This models late
publication and safe failure. The late-write and rejected-worker cases passed on
the reference DXR driver at remote commit
`794b90c83692e82c33d876cf67ae87f4b7bdaa45` (run 38309859057). All six workflows
passed. Neither result is a live Control rendering test.

## Dispatch/job routing hooks

`pt_native_job_runtime.h` now connects the existing native dispatch hook to the
exact runJob call at 359D9 and the existing worker writer hook. The dispatch
reader validates the native PSO, double-buffer selection/counter, complete table
layout, two-ray shape, and writer arguments. Scheduling registers a unique
ticket before the original runJob call. Writer publication happens after the
native fill returns but before the worker returns to the job system.

`pt_native_job_route.h` tracks the 16 native job slots with a monotonically
assigned ticket serial, exact writer arguments and one claim per job. A stale
completion cannot clear a reused slot. The routing mutex is never held while a
consumer callback executes. Error-state preservation applies at each boundary.

This bridge is inert until a session-lifetime renderer consumer registers. No
consumer is registered yet. The inline writer and native zero-identifier debug
mode are intentionally unsupported and retain native behavior. The public
registration contract does not allow replacing a consumer with pending jobs.

Still required: the consumer must reserve its private dispatch/output resources,
capture the selected TLAS and descriptors, retain all GPU dependencies, build the
table using the writer callback, publish behind the correct queue dependency,
and retire resources on a real fence. The native observer's 120-second registry
is not a production resource owner. Final lighting composition also remains
unimplemented. Do not enable private rendering solely because routing tests pass.


## Exact allocation leases and queue retirement

Scene snapshots now require frozen allocation tokens and byte ranges for the
selected TLAS and every reachable unmasked BLAS. Preflight rejects missing,
reused, overlapping, or undersized allocations and includes the AS backing
resources in the deduplicated lease list. The DXR fixture supplies its actual
BLAS/TLAS resource identities; geometry-only ownership is no longer sufficient.

`Runtime::PrepareLease` validates the complete dispatch plan and retains its
resources under one registry mutex. `LeaseBatch` keeps an independent shared
budget charge until its final references are released. Registry removal or
Stop cannot refund that charge. Whole backing buffer widths are charged once
per batch, conservatively again across batches, with a default 2 GiB cap in
addition to the diagnostic registry cap.

`pt_scene_submissions.h` accepts up to 16 retained batches. Its host must call
Submitted after the actual ExecuteCommandLists on the initialized queue and
serialize this boundary with command-list reset. Multiple batches for one
recording share a retirement fence. Unsubmitted reset can discard a batch;
submitted reset cannot. Failed signaling or device removal prevents reuse.
Destruction without completion proof intentionally quarantines references and
their budget charge for process lifetime. Recording generations must never be
reused or executed a second time.

The WARP test exercises an actual blocked queue fence, native-owner removal,
registry shutdown, budget rejection, deduplication, and unsubmitted discard.
This component is not yet wired into the renderer consumer or native queue
callbacks. COM retention does not make contents immutable, establish AS build
ordering, or retain the PSO, descriptor heaps, textures and output dependencies.
The production owner still needs a session lifetime and exact live snapshots.
Rendering remains disabled; M11 remains on hold and no user run is requested.


## Submitted bindings and session owner

The offline descriptor trace now follows the exported RenderFrame submission
sequence and applies ModifyDescriptors calls. All six views bind their opaque
reflection scene at t5 (DXIL SHA256
`c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e`).
Other ray libraries use different slots; stale unused AS descriptors elsewhere
in a table must not choose the scene. The corrected six-view evidence is in
`entrance-submitted-bindings.json`.

`pt_scene_live_owner.h` separates the future session owner from M11's timed
registry. Native resource creation/release and geometry hooks forward to it only
when explicitly started. No Start or renderer registration is issued yet.
`pt_scene_descriptors.h` tracks sparse AS descriptor identities across both copy
APIs, non-AS overwrites, and heap range reuse. Selection verifies the copied
allocation token remains current. Device hooks cover SRV/UAV/CBV writes, heap
creation, CopyDescriptors and CopyDescriptorsSimple.

The verified native CreateRay routine creates its AS SRV at 26581 BEFORE it
returns to our wrapper-level creation hook. Resolving its owner in the SRV hook
alone would always miss first creation. For native type 1, the post-create hook
seeds the descriptor at wrapper+10 only after retaining the new allocation.
This follows the hash-locked DLL's actual order; it does not revive old copied
identities when a VA is reused.

WARP coverage exercises this post-create seeding, actual heap slot selection,
copying, native-owner release/recreation at an identical VA, and heap invalidation.
The owner remains dormant pending exact AS build-version selection and the
renderer consumer. A CPU-current allocation token is NOT an AS content version
or proof of GPU dependency ordering. Neither this checkpoint nor successful
compilation makes the project ready for a user rendering test.


## Queue ordering model and remaining hard boundary

`pt_scene_order.h` models submitted AS writes, queue ordering and fence
signal/wait dependencies. A wait issued before its signal remains pending until
that exact dependency can be resolved. A dispatch cannot select a later CPU
build merely because it was most recently recorded. Multiple unordered writers
are rejected; an unknown write invalidates the older metadata. Fence signals
must form one ordered timeline even when producer queues change. Increasing
CPU-observed fence values alone do not prove that timeline: a larger signal on
an unrelated queue can satisfy a wait before the desired producer completes.

This follows Microsoft's fence synchronization contract:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
The portable tests include a late signal, an ordered producer handoff, a racing
higher signal, a cyclic wait, and allocation identity changes.

The saved overview has seven AS builds on queue 5831/list 5832, a signal of
fence 4873 to 942, and a matching wait on queue 1 before its ray dispatches.
PIX comments out that native Wait and inserts flushes for replay. These concrete
cross-queue operations rule out using CPU recording order as the live version
selector. The graph is not wired to live queue events yet. Its bounded history
also needs fence-proven checkpoint/retirement before long-running production use.

A further distinction must remain explicit: retained COM resources preserve
allocation lifetime, not immutable geometry content. Source position/index
buffers can in principle be rewritten or suballocated again without destroying
the backing resource. The live scene adapter must either establish their content
version through the relevant GPU use or preserve the needed build inputs in
private storage with correct states and ordering. The current leases do neither.
Do not enable rendering or infer readiness from a valid CPU plan and retained
pointers alone. Primary-light integration and the material compositor are also
still absent. No user test package is ready at this checkpoint.

## Submitted scene catalog and writer handoff

The following implementation now exists, still dormant until an integrated
renderer explicitly starts the session owner:

- `pt_scene_catalog.h`: immutable BLAS/TLAS build records, unknown-write
  tombstones, compaction/clone source selection at the copy's own queue cut,
  command-list recording generations, and dispatch-cut scene assembly. Reset
  discards unsubmitted records; repeated execution is rejected. Exact allocation
  identities travel with geometry and each TLAS instance's BLAS address.
- `pt_scene_capture_runtime.h`: native build/instance-upload/AS-copy adapters.
  Instance rows are copied before the native memcpy, not read back from upload
  memory. Canonical COM identity joins interfaces and retains their lifetime.
- `pt_scene_queue_runtime.h`: ExecuteCommandLists, Signal, Wait and successful
  Reset observation on the actual supplied interfaces. Nested proxy forwarding
  is observed once. Queue operations and observation share a serialization
  boundary; failed fence operations invalidate admission.
- Native queue flush interception covers all 26 direct calls to the verified
  `d3d+434a0` function. Each exact E8 target was checked against the private
  native DLL. The queue at wrapper+10 is watched before a flush, including an
  empty flush before the caller issues a fence dependency. No alternate native
  queue is substituted for the submitted proxy.
- Native job tickets now preserve the complete dispatch-time shader-table
  layout. `SnapshotWriter` copies matching native identifiers, binding rows and
  current allocation generations inside the resource-owner transaction while
  the native worker owns the source pointers.
- `pt_scene_handoff.h` joins recording, writer readiness and submission-time AS
  descriptor selection. It retains the native state object and descriptor heap,
  freezes the exact AS slot at submission (not recording), and prepares a
  complete private table plus leases only after the scene cut resolves. The
  renderer must call its Submitting callback inside the queue interception;
  this consumer registration is not active yet.

Windows CI for the queue change passed, including actual WARP Reset, Execute,
Signal and Wait forwarding and scene selection. The native capture fixture also
passed immutable instance-copy, compaction, geometry-token and submission-cut
checks. Those synthetic build descriptors are observed only; they are not sent
as AS builds to WARP. The original rendering hash gate still passes.

These modules do not solve geometry-content ownership. COM references still do
not freeze position/index/attribute data. Before dispatch, preserve required
build inputs or establish their content version and GPU states. The graph also
needs retirement/checkpointing rather than eventually exhausting bounded
history, and CPU-signaled/preexisting fence timelines need explicit admission.
The live handoff needs cancellation/queue callbacks connected to an always-
unblocking private GPU gate, private PSO/SBT/output ownership and retirement.
Native primary material/light adapters and final material composition remain
unfinished. The session owner is not started and no new rendering consumer is
registered. No full-PT user test package has been released by these changes.

## Build-time GPU geometry preservation (October 12)

The verified native buffer wrapper stores `D3D12_RESOURCE_STATES` at `+0x20`.
This is established by the native buffer creation paths (RVA `26101`, `26130`,
`26514`) and, more directly, the native buffer copy routine at RVA `26990`:
`26AD6` reads the source state, `26B28` records its transition to COPY_SOURCE,
`26B93` copies the buffer, and `26BCF` restores the exact saved state. This is
specific to the already hash-gated Steam module, not a portable inferred ABI.

`pt_geometry_snapshot.h` now records private GPU copies of index/position
allocations at the intercepted BLAS build boundary. Native wrapper identity,
allocation generation, default-heap properties and readable state are validated
under the resource-owner lock. Unsupported states/queues and exhausted budgets
reject before recording any GPU commands. Each distinct backing allocation is
copied once per build; the source state is restored exactly, and private buffers
finish in COMMON for subsequent buffer read promotion. No PSO, descriptor heap,
root argument or native shader table is changed by this operation.

Frozen geometry metadata carries the copies through AS compaction and submitted
scene selection. Private-table conversion first validates the original native
root layout, then redirects index/position addresses to the retained copies.
Released/reused source position addresses therefore cannot substitute new data.
Original source geometry is not retained as a dispatch dependency; attributes
and acceleration structures still require exact live allocation identities.

Copy-source references survive recording until Reset, and submitted copies also
retain their sources through a private fence signalled after the actual native
ExecuteCommandLists. Failed completion proof quarantines ownership. Native list
replay remains rejected for scene admission. Private destination references are
also retained by dispatch plans and fence-owned lease batches.

Validation includes an actual WARP copy followed by overwriting/releasing the
source, delayed queue execution behind a fence, and readback comparison with the
original bytes. Pure table tests exercise original GPU-address reuse, redirected
private roots, exact index generation, missing ownership and R16 padding.

This closes the allocation-lifetime-versus-content distinction for admitted
BLAS geometry copies, but does not enable the full renderer. The copy archive
currently has a 512 MiB bound and catalog history still needs retirement for
sustained scenes with dynamic BLAS updates. Private dispatch gating, primary
lighting bindings, material composition and renderer activation remain separate
unfinished work. There is no newly approved in-game test build from these checks.
