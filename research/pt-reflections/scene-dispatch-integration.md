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

### Completed-queue history checkpoints

The queue observer now fences every forwarded queue tail and polls those fences
before later queue operations. After 128 observed operations, a checkpoint is
eligible only when all tracked queue tails have actually completed and no
submitted handoff ticket still needs an old cut. It never stalls a queue or
waits on a native writer. Completed fence values survive the graph rebase, and
new queues inherit the same CPU-proven completed prefix.

The catalog materializes compaction sources at their original source cuts before
removing older versions. It keeps current snapshots for live allocation tokens,
retains unsubmitted recording snapshots, and releases obsolete GPU-copy owners.
A regression performs 20,000 update/checkpoint cycles, verifies old signal waits
and newly encountered queues, protects outstanding tickets, and checks that a
compacted source survives until its last dependent geometry is replaced.
Continuous GPU backlog can delay reclamation; the archive still refuses new
copies at its fixed budget instead of freeing live data. This remains distinct
from renderer activation and does not make the PT lighting/composition ready.

### Immutable private dispatch and later-submission validation

`pt_scene_pipeline.h` now creates the production private 60-byte-payload state
object and maps native opaque/alpha identifiers to its four private hit groups.
The M4/M5 host uses this factory; its Windows builds pass. GitHub does not execute
DXR hardware tests, so successful fixture compilation is not a game rendering
result.

`pt_scene_dispatch.h` prepares the complete upload SBT before recording GPU use,
retaining the private pipeline, descriptor heap and frozen geometry. Its record
operation preserves the admitted global root and restores the native compute
PSO and table. Both 49-descriptor ranges must be in the retained heap, correctly
aligned, disjoint, and in bounds. A scene lease can now own this dispatch through
its submission fence. The renderer must still prove descriptor contents, all
other root arguments, and the current native pass before calling it.

The handoff can retain an immutable `Prepared` proof after consuming the original
native-writer ticket. A later lighting-pass ticket is checked after catalog
submission and before forwarding Execute. Validation regenerates the complete
plan under the owner lock and compares the TLAS version, every referenced BLAS
version, allocation identities, pipeline identities and all private SBT bytes.
An in-place BLAS update is rejected even when its table bytes do not change.
Missing or unresolved queue history never authorizes dispatch. This removes the
need to keep the original historical ticket alive solely for revalidation.

This is an API for a later-pass integration, not an active consumer. The host
must still connect actual submission callbacks and solve cancellation/replay
without changing an upload enable flag that an earlier GPU execution may still
read. Simply writing such a flag to zero on replay is not a valid solution.
Native lighting/material adapters, composition and renderer activation remain
unfinished. No user game run is requested by this checkpoint.

### Renderer submission callbacks and engine-frame identity

The queue interceptor now supports one session-lifetime renderer consumer. Its
callbacks run before catalog submission (descriptor selection), after catalog
submission but before native Execute (scene validation), after native Execute
(retirement), and after successful Reset (cancellation). Native execution is
forwarded even if scene admission fails. Nested private fence signals forward
without recursively observing another native queue operation. Reset and the
complete submission boundary share the queue mutex.

The WARP fixture exercises these callback phases against a real queue/list and
checks that the selected scene changes from pending to ready at the correct
boundary. Its private retirement signal runs after Execute. Registration is
single-owner; callbacks must have session lifetime in production.

Native RVA 3570f increments the shader-table counter on a ray dispatch. That
counter is therefore not a frame ID. Tickets now separately freeze the established
engine frame value at d3d RVA 136c68 (the same field used by camera and RR capture;
native setter/getter RVAs 239b0/239c0). A guarded double read rejects a frame change
during capture. Worker completion carries the recording frame, even when the
engine has already advanced. Later-submission proof checking additionally requires
the same frame and dimensions. Candidate lookup rejects duplicates instead of
selecting the newest ticket. Frame/pipeline/extent discovery is not sufficient
view proof: the live pass adapter must still correlate camera/root bindings.

These callback APIs are implemented, but no complete PT renderer consumer is
registered yet. Lighting and graphics pass hookup remains unfinished.

### Native graphics pipeline companions and root-layout proof

The device's native CreateGraphicsPipelineState path now has an optional
companion creation observer. It returns the original native HRESULT, object and
Win32 error state unchanged. Once the scene owner is active, possible reviewed
pixel shaders are SHA256-verified and patched into a separate graphics PSO with
all original shaders, root signature, fixed-function state and target formats
preserved. Only the pixel bytecode changes; the native cached-PSO blob is cleared.
The companion registry owns canonical COM identities and is bounded to 512 PSOs.
A companion is never selected merely because creation succeeded.

CreateRootSignature now records the exact reviewed native graphics and compute
layouts from their serialized version-1.0 descriptions. Graphics root 1 is the
pixel 49-slot table; root 0 is the vertex table. The 18-parameter graphics layout
also has hull/domain b0-based tables, unlike the vertex/pixel b4-based tables.
Every range, register space, visibility, append offset, root CBV and signature
flag must match. Unknown versions/layouts and signatures created before the
observer was installed are not admitted. The graphics companion factory requires
an owned layout proof for the exact root identity supplied in its PSO description.
This avoids using a matching pixel shader as evidence of root-parameter order.

The Windows root-layout fixture mutates every binding, checks serialized roots
on WARP and verifies that proofs retain their root objects. These checks do not
execute the nine captured pixel shaders on WARP: those private bytecodes are not
published to the repository. Shader-container/token verification remains a local
check against the captured originals. Draw binding, output ownership and the
private light transport producer still need to be connected before activation.

### Queue-ordered surface output and replay fallback

`pt_surface_output.h` owns a separate RGBA32F output for one frame/view, CPU
SRV/UAV descriptors, a private clear heap, immutable command lists, dependencies
and a retirement fence. Creation enqueues a GPU clear before any consumer. An
unproduced pixel has zero admission, so the composition adapter keeps native
radiance. A private producer records into its own command list with explicit
read/UAV/read transitions. Recording failure never executes partial commands.
The producer submits at most once, after the renderer has verified its scene cut.

Replay fallback clears that output on its pinned direct queue before the replay.
Previous consumers complete before the clear; later consumers see zero admission.
The clear command list is immutable and does not reset an in-flight allocator.
Invalidation also prevents a prepared but not-yet-submitted producer from writing
late. This addresses the earlier unsafe proposal to edit a CPU upload flag while
GPU consumers might still read it. It does not authorize a consumer on another
queue: admission must establish the actual pinned queue and native frame/view.

Each output is capped at 256 MiB. The renderer must additionally bound the number
of outstanding outputs and retain each through BOTH native recording lifetime
(until Reset) and submitted GPU completion. The output's own fence covers its
private clear/producer commands; consumer leases must cover native draws. Failed
signals or destruction before private completion quarantine all dependencies.
The WARP fixture queues all operations behind a gate, reads back zero/produced/
invalidated pixels in order, and checks wrong-queue, duplicate/late-producer and
retained-dependency behavior. Live draw binding and the light transport encoder
are still required; this class alone is not a playable integration.

### Actual command-list root arguments

`pt_command_bindings.h` intercepts native SetPipelineState, descriptor heaps,
compute/graphics root signatures, root descriptor tables and root CBVs. The
CreateCommandList observer installs it on native direct/compute lists while the
scene owner is active. Canonical COM identity joins interface aliases. Reset
clears both graphics and compute state; it chains with the scene observer's
existing Reset interceptor. Heap changes invalidate recorded table arguments,
while root CBVs survive. Repeating the same root signature preserves arguments.

Snapshots require an owned, exact native root-layout proof, a caller-specified
set of populated arguments, aligned CBVs, and complete descriptor ranges inside
the currently bound heaps (including all 244000 material descriptors). This
provides the actual b0 GPU address for matching the ray pass, primary lighting,
and pixel pass; frame number alone is not view identity. It does not prove CBV
contents, descriptor contents, native texture lifetime, or GPU ordering.

Private output-list creation/recording uses a thread-local exclusion scope so
private bindings cannot overwrite native state. The WARP binding fixture covers
real intercepted setter calls, aliases, exact root layouts, heap replacement,
bounded table addresses, preserved CBVs, private-call exclusion and chained
Reset. Native lists created before activation remain unadmitted until explicitly
watched and rebound. No renderer consumer is enabled by this observer alone.

The binding observer now also provides actual Dispatch/DrawInstanced/
DrawIndexedInstanced callbacks. Native calls always forward with their original
arguments, including signed base-vertex offsets. Draw callbacks bracket the
native call so a renderer can restore its temporary pixel table/PSO immediately.
Nested private draw/setter calls do not recurse into the consumer. A separate
COM-ABI forwarding fixture checks callback order, original arguments, nested
exclusion and Win32 error preservation without issuing invalid GPU draws.

The state-object/compute creation observers now identify the exact native opaque
DXR library (39008 bytes, c2e4c0c4...) and primary lighting program (71144 bytes,
a9e5b544...). Their registries join canonical PSO identity to the verified compute
root layout and retain both. Unknown roots, collections without a directly
verified global root, unknown shaders and excess entries remain unadmitted.
These are the pass selectors for the pending renderer consumer; the active P2/G3
selection and native return objects are unchanged.

### CPU-source descriptor mirrors

`pt_table_mirror.h` mirrors the native 49-slot table suffix after the 244000-entry
material prefix. It observes native CopyDescriptorsSimple/CopyDescriptors and
direct CBV/SRV/UAV creation under one serialization gate. It never copies from a
shader-visible source heap. Private mirror/snapshot writes are excluded from
observation. Unsupported aggregate copies invalidate observed slots rather than
leaving stale values eligible for a snapshot.

The mirror is bounded to 16 heaps and 2097152 suffix descriptors. Each admitted
snapshot owns a separate CPU descriptor heap, fills unused slots from typed null
defaults, and requires every shader-used native slot to have an observed write.
Per-slot versions detect later native destination changes; changing only the
original CPU source does not alter an existing snapshot. Required masks for all
nine surface profiles were extracted from their captured RDEF bindings and are
stored with the reviewed shader identities.

The WARP fixture covers a bulk copy crossing the material/table boundary,
multiple destination/source ranges, direct view creation, old/new snapshots,
private-write exclusion and GPU reads through both copied tables. Descriptor
mirroring proves descriptor values, not backing-resource lifetime or resource
states. The renderer still must establish those at its admitted native pass and
must not reuse a copied graphics table across native descriptor mutation.

The reviewed allocator at native RVA 1aa00 starts each heap's dynamic suffix at
0x3b920 and caps the heap at 0x7b920 entries. It advances the global allocation
cursor and associates the allocation with the native fence token. On rollover
it creates/recycles a heap and copies the material prefix. Inspection through
RVA 1acea shows no command-list SetDescriptorHeaps call in the allocator itself;
rejecting a returned different heap therefore does not change the command list's
currently bound heaps. The private table still requires the same native heap.

### Inline producer, view identity and primary shader

The output owner now supports a producer recorded inline immediately after the
native primary lighting dispatch. A separate immutable queue-ordered clear arms
alpha one only at an admitted submission; a rejected/replayed producer sees alpha
zero and its raygen returns before TraceRay. Native consumer submissions extend
the output's own retirement fence. Pending healthy outputs retire from an
intrusive deferred list; failed/unknown fences quarantine within a shared 1 GiB
budget. No frame-count retirement or allocator reset is used.

The new `pt_primary_path.hlsl` compiles as a separate 60-byte payload library. It
uses four surface hits, delta-light NEE with traced alpha-aware visibility,
GGX/VNDF plus Lambert sampling, Russian roulette, safe geometry spawn points,
secondary emission and sharp environment misses. The primary material/depth must
match the raster GBuffer. Primary emission is excluded for the composition patch.
Special BRDFs and invalid materials reject the whole pixel. The initial transport
uses geometric normals and `kd = textureColor * (1-F0)`; this is a new material
model, not a claim of native material/normal-map parity. Point/spot/sun remain
delta sources, not area emitters. No denoising or temporal accumulation is added
by this library alone. Environment t34 must come from the same native view; it is
not a substitute ambient constant.

The capture establishes different b0 upload addresses for opaque rays
(4787+9728), primary lighting (4716+20736) and material drawing (4753+3584).
Comparing addresses would reject these valid same-view passes. `pt_native_view.h`
therefore compares the first 288 bytes of native camera/resolution data and the
current-frame field at byte1112. Reading these requires a retained UPLOAD
allocation at the boundary; arbitrary GPU memory is never dereferenced. Missing
ownership, dimensions or camera data reject the match. The runtime consumer and
activation still must connect these pieces before this becomes a playable build.

### Connected session consumer (M12 work in progress)

`pt_primary_runtime.in.h` now connects native job completion, verified opaque and
primary pass identities, camera-content matching, environment capture, private
SBT preparation, inline ray dispatch, companion surface draws and native queue
retirement. An output can be allocated by a material recording before the
lighting recording, so worker recording order does not choose GPU order. The
native direct queue is pinned from the hash-locked TLS context. Eight outputs
maximum and a 1 GiB output allocation budget bound retention. Every modified
draw restores its native PSO and pixel table immediately afterward. Native PSOs,
unknown/special materials, alpha, fog, media and primary emission remain native.
M11 timed capture is disabled; the session hooks persist without capture files.

Submission revalidates the exact AS cut, writer SBT, view data, descriptor versions
and F10 state. Output alpha is armed only before an admitted native submission.
A replay clears alpha in the same queue before executing. Changed surface table
values refresh the private clone from the CPU mirror, including for native
fallback. Resource use is fenced after native producer/consumer execution, with
one signal per output per submission rather than one per draw.

An additional lifetime correction is essential: selecting an old reachable AS
version does not exclude a later unordered write on another queue. The order
graph now rejects such a cut; a future write must be a proven descendant. After
an armed private dispatch submission, a session fence on the native direct queue
extends the scene read lifetime. Subsequent submissions on other native queues
wait for that fence; these private Signal/Wait edges are also entered into the
scene catalog. This serializes later native AS/attribute writes behind PT use and
may reduce asynchronous overlap. It does not copy or rebuild native acceleration
structures. All native DIRECT/COMPUTE/COPY list resets must be observed, including
lists without scene operations, to keep generations correct on subsequent reuse.

This wiring is under compilation/integration verification. It is not a measured
in-game image, performance, coverage or RR-guide equivalence result. The session
requires the new primary sidecar and exact supported native modules. Missing
prerequisites leave approved P2/G3 available rather than producing a fake PT
success. No component-only result is advertised as the completed playable build.

### M12 packaging and runtime verification

The connected consumer compiled successfully in Windows workflow 38330989467
(commit f7f696ad161615463c152b7c0d8999bfd28a0d40). The preceding WARP tests also
passed actual inline arm/dispatch/replay behavior and deferred output retirement.
No game rendering result is claimed from these checks.

M12 now carries sparse GPU counters in private u6. One pixel per 16x16 block
reports progression through dimensions, primary hit, material/depth match and
valid path completion. The counter buffer is cleared and copied on the native
producer list and read only after the output's native submission fence completes.
The collector distinguishes a CPU dispatch log from actual covered GPU samples.
This telemetry is part of the integrated rendering build, not a separate capture
run, and it is not an exact full-frame coverage percentage.

At sustained GPU load, idle-only journal retirement could fail to run. After
4096 queue operations, capture may drain already-queued tails for at most one
second, but only when all waits have queued signals and no submitted scene ticket
needs its historical cut. It never waits on an unscheduled writer or signal. A
session-lifetime event handles delayed completion notifications safely. This
bounded housekeeping may cause a periodic stall in this experimental build.
Failed primary attempts release their native handoff tickets so they cannot keep
old history alive indefinitely.

The M12 package includes the proxy, primary library, all matching approved P2/G3
shader sidecars, build provenance, checksums, instructions and one log collector.
It replaces the held M11 diagnostic package in this branch's workflow artifacts.
The M11 timed recording stays disabled. The next user test is a single in-game
F10 comparison with GPU coverage reporting, not another CPU-only diagnostic.

### M12R1: response to failed M12 game run

The uploaded M12 run reported 3–5 FPS with SessionReady=true but no submitted
primary dispatch or GPU coverage. It logged repeated surface_view_or_context
rejections. FG was off. Do not call M12 an in-game rendering success.

A local sustained-history fixture (100 updates of 32 AS keys, 3,200 writes)
reproduced a 957.7 ms CompletedCheckpoint scan. Cached per-queue vector clocks
reduced the identical case to 0.36 ms locally. 32,000 selections and 200
checkpoint histories matched the old implementation in differential testing.
This is an identified CPU bottleneck, not a proven attribution of all in-game
frame time. The blocking checkpoint wait was removed; idle-only checkpoint
retries are bounded to once per 128 queue operations, including failed attempts.
Long continuously busy sessions may hit the catalog limit and fall back; partial
completed-prefix retirement remains future work. Safety rejection is retained.

The captured source creates the 64-KiB constant pool directly through committed
UPLOAD resources (e.g. resource 4753); NativeBuffer-only ownership cannot cover
that pool. M12R1 observes committed 64-KiB UPLOAD buffers at device creation,
retains a bounded 128-MiB session pool, and resolves exact root-CBV slices from
those owned buffers. WARP validates mapped slices, bounds, and ownership.
Unrelated draw/dispatch PSOs are filtered before taking the queue lock. Specific
view/binding/native-job failures and CPU build/checkpoint timings are logged.

F10 opens the overlay; the PT Reflections overlay toggle controls admission.
M12 instructions claiming F10 directly toggles PT were wrong. The collector now
uses plain strings, preventing Get-Content provider metadata from inflating JSON.
In-game GPU coverage, image quality, and performance remain unverified for R1.

The native primary DXBC unprojects pixels using b0[0].zw (InvScreenRes).
M12 incorrectly used b0[1] (OutputRes/InvOutputRes) for host dimensions and ray
unprojection. The uploaded log confirms 2560x1440 render / 3840x2160 output.
R1 uses ScreenRes/InvScreenRes and tests unequal render/output extents.
