# R428 integration in progress

R427 v2 completed the owned-recording order checkpoint. Do not request another
ordering, pipeline, or compact-preview capture. No enabled gameplay build yet.

Added GIRecordedHeapIdentity and ReadNativeGIRecordedHeaps. The snapshot records
both native heap records, actual bound heap objects, generation/cursor/epoch and
the native device fence record/object/epoch. Revalidation reads the saved records
directly, with no submission-thread TLS lookup. It rejects recycling, changed
fence identities, a completed admission epoch, device removal, frame changes and
cursor rollback; append-only cursor movement is accepted. The native cursor is
read as 32 bits, avoiding adjacent padding. Failed reads expose no partial sample.

These are necessary identity checks, not a scene lock or descriptor-content lease.
They deliberately do not set sceneAndDescriptorLease or enable the replacement.
Host code must retain records and exclude mutation across read/use. Admission
requires exact supported native build and original recording bindings. The new
reader receives the previously captured identities; it never discovers a current
heap itself.

Validation: portable C++ regression passes with GCC -Wall -Wextra -Werror.
Windows core run43157678949/job130981301018 passed, including the native-layout test. It exercises the
native layout reader and inaccessible-pointer rejection in addition to the
portable wrong-context/retirement cases.

Still required before a user visual build: native scene scope integration,
exact hit/output/SBT acquisition, bounded reusable invocation storage, actual
producer/consumer queue/reset callbacks and completion/failure publication.
No new noise or FPS result is claimed.

## Follow-up integration corrections

The exact D3D binary was recovered from an existing uploaded DLL (SHA256
cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5).
The native retirement audit passes. AcquireNativeGICurrentSbt now selects the
original ray call's actual alternating 0x50-byte slot using the context+a8
counter, then validates its resource/base/capacity. It must be called inside
that native dispatch scope; retaining a later global current table is forbidden.
Tests cover odd/even selection, stale-slot rejection and invalid counters.

Fixed an integration bug in D3D12SplitOwnerRetirement: an unrelated foreign
queue submission previously quarantined all owners. Global Execute hooks must
observe unrelated work without disabling GI. Foreign-queue work now quarantines
only invocations actually referenced by its Execute list; an unknown recording
generation quarantines owners referencing that list, not unrelated owners.
An empty submission is a no-op. Actual owned wrong-queue or missing-generation
execution still fails closed. Expanded WARP test checks both normal unrelated
traffic and these owned failure paths. Windows core run43158329850 passed for this delta.

These corrections do not enable the frozen GI dispatch. The previous diagnostic
package remains a diagnostic, even if the full packaging workflow succeeds.
Do not label its automatically generated artifact as an R428 visual fix.

## Allocation reuse

Added explicit RearmAfterRetirement to the SBT, ray constants and frozen raygen
record owners. These operations are only legal on an invocation returned by the
retirement pool after both recording seals and its final consumer fence. They
are not independent fence checks. Before reinitialization no stale dispatch,
resource handle or raygen identity is exposed. SBT storage grows transactionally
only if required; constants and raygen records retain their allocations. Shared
pipelines remain immutable. Recycled DEFAULT storage transitions from COMMON at
its next recording; UPLOAD raygen records are rewritten only after retirement.

Expanded GPU fixtures execute another invocation with different native data,
frame/epoch and parity, require identical GPU allocation addresses, and read back
the fresh bytes/zero padding. They also reject duplicate rearm and stale metadata.
The SBT test now uses COMMON at separate Execute boundaries, matching Microsoft's
buffer decay contract:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12

This remains integration work, not a live gameplay fix. No user capture needed.

Allocation-reuse validation passed Windows core run43159004821. No build has
exercised guided rays in the game yet; WARP validates compute/storage, not DXR.

## Full native render scope

Recovered the native callback slot631DA0 -> thunk13E860 -> body128D20. Exact
thunk bytes and the adjacent MSVC RTTI identify a std::function<void(
OrderSolverPassData*)> invoker. It adds8 to the closure pointer and dereferences
the pass-argument pointer before entering the full render body. The existing
scene-window audit now reproduces these assertions against the exact renderer.

Added a full-invocation wrapper, compiled in probe.cpp and installed only with
CONTROLTOH_GI_FROZEN_RENDER=1. That flag currently installs the scope ONLY; it
still does not enable guided dispatch. It preserves the original thunk, records
an exact token/frame/owner/pass in TLS, restores the previous scope, and invokes
host cleanup in __finally on normal return or native exception. It establishes
an invocation identity and cleanup boundary, NOT writer exclusion. Regression
fixture invokes the real instruction thunk with two arguments and checks both
normal and exception unwinding. Windows core run43159995782 passed for this scope delta.

Outstanding remains the native writer/descriptor ownership gate and live host
transaction wiring. Do not replace that gate with a true boolean, silently ship
the diagnostic package, or request another already-completed ordering capture.

Recyclable snapshot/work owners are now noncopyable. Otherwise a copied ComPtr-
backed wrapper could rearm and overwrite an allocation still owned by another
invocation. Compile-time assertions cover this ownership rule. Shared immutable
pipeline bundles remain shareable. Windows core run43160430664 and full DLL
run43160430673 both passed for commit3dd597e (all60 build steps successful).
This establishes compilation/component validation, not in-game guided quality.


## Native heap allocation and complementary recovery

Expanded audit-gi-native-retirement.py against exact D3D SHA cceb99cb... .
The current resource heap fast path appends only while generation matches and
capacity remains. Generation changes select a new or fence-completed pool
record; the material prefix refresh follows that gate. Output is checked into
native-retirement-r428.json. This supports retaining the recorded heap, not
re-resolving current TLS at submission. It does not establish ownership of its
referenced resources or protection against geometry/AS writes.

Added D3D12FrozenCompletion::RecordOrRecover. It chooses before any completion
commands: ordinary guided completion when all admission checks pass, otherwise
uniform missing-half rays only when the independent replay admission still
passes. It restores the original hit states and shade bindings, returning
NativeShadeRequired so the host can run full native GI6 shading exactly once.
No full ray retry occurs. A failed/partial guided recording is not retried.
Missing scene ownership, producer submission, valid retained heaps, SBT/constant
identity, or hit resources still rejects recovery. The host must claim the
consumer once through the retirement owner before calling this entry point.

WARP can check rejection and compile this path but cannot execute DXR recovery.
It is not wired into the live game yet and does not make an enabled build ready.
Native writer exclusion, complete host integration, and an actual DXR runtime
validation remain outstanding. No repeat user ordering capture is warranted.


## Earlier preparation boundary

The full preparation callback starts at renderer123D60, before graphics flush
124130 and scene-fence signal124152. Material hook12491D runs later. Therefore
blocking only material/prepare-worker/record-worker hooks is insufficient: the
async writer could wait for a native graphics signal placed BEFORE delayed GI
rays. Any scene gate must block admission at this earlier preparation boundary.

Recovered callback slot632270 -> thunk13DE60 -> body123D60..124A2B; exact thunk,
RTTI, closure owner offset28h and unwind extent are now reproduced by the static
audit. Added gi_native_prepare_scope.h, installed beside the render scope only
under the still-disabled CONTROLTOH_GI_FROZEN_RENDER flag. It forwards both native
arguments unchanged to the thunk, notifies before any native preparation and
runs cleanup on normal/exception exit. It notifies even with unreadable metadata
so a future gate cannot treat a failed read as permission to bypass exclusion.

This boundary hook does NOT implement writer exclusion yet. The future gate
must cover preparation admission through actual consumer submission (or a
proven cancelled/reset consumer), preserve native graphics/async ordering, and
handle exceptional termination without reusing recorded resources. No new user
capture is needed for this finding. Recovery core run43162577274 passed for
commit10ef8ac; full DLL validation is still in progress for that commit.


## Descriptor-heap change at the consumer

Added D3D12SavedHeapSwitch and used it for both guided completion and uniform
complement recovery. A producer/consumer heap change no longer automatically
rejects an otherwise valid replay. The helper deliberately binds the retained
ray heaps, restores ray bindings, and then restores the consumer heaps plus
its current graphics descriptor tables and compute bindings. Returning to the
same heap does not itself resurrect graphics tables invalidated by a switch.
All admission checks run before recording; unknown graphics coverage still
rejects a heap change. Matching heaps require no switch or graphics replay.

The new WARP fixture executes two compute dispatches using different CBV heaps
and then a real graphics draw without rebinding its table. It reads back both
compute values and the graphics pixel to verify restoration. Validation pending.
The earlier prepare hook compile error was an ADL ambiguity caused by the shared
Scope type; commit48afa3b qualifies its Read helper. No live dispatch is enabled.


## Preparation gate and joined-scene admission

Added NativeSceneWindow and gi_native_scene_window.h. The native adapter hooks
only the exact scene join inside the full render invocation and installs entry/
exit callbacks on the earlier full preparation wrapper. It is still under the
disabled FROZEN_RENDER startup option. Preparation admission waits while a
reader is held. Native preparations remain concurrent with each other. A reader
must match the invocation, owner scene and epoch sampled BEFORE the actual native
join; any intervening preparation rejects admission even if it already returned
and its asynchronous jobs are still running. No lease is inferred from pointer
comparison alone. A conflicting diagnostic join hook makes installation fail.

The native host still must acquire/release this gate around the actual split
transaction and retain its GPU objects. Release is legal only after actual
consumer Execute or a proven cancellation/reset that cannot execute delayed
rays. Do not release blindly on render-scope exit. The pinned invocation must
not recursively enter preparation while holding its reader on the same thread.
This gate does not certify other unhooked scene writers/resource suballocators.

Portable multithreaded tests passed with -Wall -Wextra -Werror, including a
preparation actually waiting at the gate, wrong release token, stale join epochs
and concurrent native writers. Windows adapter tests check invocation/scene
identity, native join forwarding, epoch changes and exception invalidation.
Windows core run43163751958 passed the heap switch + real graphics draw for
commit77ba98f. Earlier scope correction passed core43163400558. Gate Windows
validation pending; no gameplay ray dispatch or visual fix is enabled yet.


## Validated checkpoint 2026-11-12

Code commit43b1f7da9efcc8d27adae806098c449e3bb1196c passed Windows core
run43164260362 (job131002322761) and full DLL run43164260393
(job131002389574). Job logs explicitly report passing native render/prepare
thunks, multithreaded preparation admission, native scene-join adapter, and two
heap compute dispatches followed by a graphics draw. The full DLL also passed
its existing build/package validations. No new visual package was delivered.

Remaining concrete integration work:
- Whole-invocation owner connects this gate to original-ray admission, saved
  native heap records/SBT/constants/hit resources, and exact GI6 consumption.
- Independent queue observer and successful Reset observer connect the bounded
  retirement owner. Release the scene gate at actual consumer submission or a
  proven cancelled/reset consumer; preserve GPU ownership through final fence
  and both recording seals. Do not overwrite the existing NRD observer set.
- Suppress original work only according to the full transaction outcome; no
  full-ray retry after a partial split. Complement recovery must shade once.
- Explicitly mute all injected dispatch/binding observers, including native
  lighting PSO selection/timing (gi_compute_hooks::Mute alone covers only its
  own tracker, not every native_lighting hook).
- Native frame fence/deferred release and the preparation gate must cover all
  referenced geometry/AS/descriptor resource contents; the gate alone is not
  permission to set sceneAndDescriptorLease=true.
- Only then enable the live path in a deliberately named visual package.

Existing R427 v2 captures show2560x1440 GI, same root/heap pair at all24 phase
pairs, but22 distinct producer lists and22 distinct consumer lists in the window.
That does NOT prove how soon both lists reset. Keep bounded allocation/reuse
accounting and report pool starvation; do not assume a fixed two-list ring or
silently raise the memory budget. Existing captures remain sufficient for the
previous ordering questions; do not ask the user to repeat them.


## Submission/reset bridge and injected command isolation

Added a split-GI submission observer independent of NRD/diagnostic callbacks.
It captures one callback set across Execute, marks success only after native
Execute returns, and notifies once on normal completion or C++ unwind. A failed
return is not GPU completion evidence. The scope itself never signals a fence.
Added a separate successful-recording-reset callback so the split owner can
seal recordings without displacing another feature's observer.

Muted injected compute now bypasses native lighting PSO selection, metadata,
profiling and dispatch replacement as well as the binding tracker. Previously
tracker muting alone still allowed those native lighting hooks to process
injected guide/evaluator kernels. Forwarding preserves the exact PSO and dispatch
dimensions and leaves the outer native binding metadata unchanged.

Tests cover matching callback/token despite observer changes, exactly-once
completion, failed Execute notification, coexistence of reset observers, failed
Reset not sealing, and nested injected forwarding without native observation.
Windows validation pending. These bridges do not yet connect a complete live
split invocation owner or enable the guided rendering path.

## Submission ownership connection

Added a bounded opaque-token router connecting the Execute callback bridge to
D3D12SplitOwnerRetirement. Actual native Execute occurs outside its bookkeeping
lock; admission, consumer claim, Reset and retirement use the same lock. Tickets
are retained independently of TLS. Failed Execute explicitly quarantines its
batch without signalling an empty completion fence. No callbacks are installed
by constructing this helper; the future live host must publish it after setup.

Added SceneReleaseReady, separate from allocation retirement. Every recording
that can still read the scene must have executed or been discarded by successful
Reset; a missing consumer requires explicit cancellation. Pending submissions
and quarantined owners never release the scene gate. Submitted consumers permit
native preparation admission before the GPU finishes, relying on the audited
same-queue native preparation signal/wait. GPU allocations still remain pinned
through both recording seals and the final private fence.

Portable lifetime and owner tests pass. Expanded WARP fixture connects the real
opaque callbacks to the router, submits two lists, tests the release boundary,
then requires both actual Reset hooks plus final completion to reclaim ownership.
Windows validation pending. The live ray/scene-resource host remains unfinished;
this is not yet an enabled visual build.

Additional pinned-binary inspection: D3D28210 is exported allocRaytracingBuffer;
it releases the previous wrapper, calls26360 (CreateCommittedResource at26488),
then26600. The2AE34 allocation is inside device initialization and initializes
the two8MiB native SBT slots at context+0/50, not a BLAS suballocation arena.
These findings narrow the allocation audit but do not establish coverage of
renderer-side in-place geometry updates or AS suballocation reuse.

## Windows validation and native writer inventory

Commitbc6861d passed core43206721683 and full DLL43206721489
(job131143629891). Full logs explicitly pass injected PSO/dispatch forwarding
without native observation in both fixture configurations. Commit2bd74b5 adds
the ownership router and passes core43208251590/job131148842148, including its
real callback-to-owner two-list submit/reset fixture. Full DLL43208251572 is
still in progress at this checkpoint.

Recovered the exact renderer from existing Library file
libfile_92683e01aa6881918adcb950f9a57327 (renderer_rmdwin10_f(1).dll).
The similarly named file in Control Epic Compatiable has SHA85baaf70... and is
NOT the pinned renderer; it was not used for address assertions. The exact
SHAebfb5b47... copy is now local at ../evidence-renderer/renderer_rmdwin10_f(1).dll.

Added reproducible audit-gi-native-writers.py and native-writers-r428.json.
All four IAT-relative AS write sites in the pinned renderer are accounted for:
two BLAS builds, one TLAS build and one compact tail call. The deformed-geometry
vtable points update+28 to A67B0 (which dispatches compute at A6BA6), build+30 to
A6C00; the compactable-geometry table points build+30 to150FA0 and finalize+38 to
151040. Existing scene worker A89C0 has corresponding update/build/finalize
virtual loops before its TLAS build. Mutated writer instructions are rejected.

This establishes these routes, not exclusivity of their callers. Register-
indirect call coverage, deferred allocation jobs and pool slot reuse/source
geometry writes still need closure. No sceneAndDescriptorLease was fabricated,
no guided dispatch enabled and no new user capture requested.


## Native texture acquisition and shared pipeline access

Full DLL43208251572/job131148841904 passed for code2bd74b5. Both core
and full Windows builds now validate the submission ownership checkpoint.

Added D3D12NativeGITextures. The producer reads the actual owner+240 bundle's
first three NativeTextureContainer pointers, retains their actual GPU resources,
and requires native UAV tracking, one layer/sample/mip, matching dimensions and
device, and no alias/manual-state/simultaneous-access special case. GI6 rechecks
the same containers/native wrappers/trackers/resources and returns the exact
native read states; raw output is acquired at GI6 from its container, not a later
ShaderTexture global. Partial acquisition publishes nothing. No broad descriptor
or resource registry is introduced. This retains texture objects only; the host
still needs its independent scene/content lease and recording lifetime.

The immutable prepared GI6 pipeline bundle is now available to producer code
only with the same actual native root, and to consumer code through the actual
GI6 PSO attachment. The bounded latest-bundle cache holds one shared reference;
existing invocation references remain valid when a new native PSO is prepared.
No per-frame pipeline compilation or root/PSO ownership cycle is introduced.

Added WARP checks with real textures and native-layout wrappers for correct
source/output selection, state transitions, wrapper/resource changes, aliasing,
manual state, extent mismatch and transactional rejection. Windows validation
pending. Live transaction activation still not enabled.

## Whole invocation owner

D3D12FrozenInvocation now assembles scratch, copied SBT/constants, immutable
raygen records, retained hit/output textures, original ray/shade bindings and
shared pipelines into the single object retained by the submission owner pool.
Preparation records no commands. Producer and consumer are one-shot operations;
consumer admission requires the independently supplied actual producer-submit
and native scene-lease evidence, same frame/epoch, and original textures. A
missing raw output can select the existing uniform-complement/native-shade
recovery, never a whole native ray retry. Recorded partial failures remain the
host's quarantine responsibility. The owner does not create a scene lease.

Storage accounting includes actual committed constant, shader-table and raygen
allocations. Rearm is exposed only for whole owners returned by retirement Take;
unchanged device/pipeline/extent can retain their allocations. Source wrappers
and output references are refreshed for each invocation. Live hook calls into
this owner still need connecting after scene/content ownership is established.

The new texture test initially failed because the fixture changed from the
inline tracker to an external tracker BETWEEN ray and shade. That rejection was
correct: tracker identity changes invalidate the captured source. The fixture
now establishes the external tracker before producer acquisition and retains it
through consumption; no production admission was loosened to pass the test.

## Command-list destruction and bounded reuse

Core43212437025/job131163187841 passed for a4ccaa1, including the native texture
acquisition fixture and compilation/rejection of the complete invocation owner.

The R427 v2 evidence contains22 distinct producer/consumer list addresses in24
frames. That does not prove Reset frequency or destruction, but a pool must not
silently assume every list resets. Added optional admission-time registration
through Microsoft's ID3DDestructionNotifier. It registers once per tracking State,
keeps no notifier/list reference cycle, and carries only a raw identity and atomic
recording generation. The callback never dereferences the dying command list.
Successful Reset refreshes that generation. Replacing/removing private data does
not invoke the callback or fabricate destruction. Its small independent context
survives metadata removal until genuine destruction, with a4096-context cap.

Actual destruction is an alternative recording seal; it is NOT a GPU completion
signal. The same private consumer fence must still complete before allocation
reuse. A missing notifier interface rejects observation; it never uses the
IUnknown::Release diagnostic return count as a substitute. No new Release hook
or per-dispatch query is introduced. The live host must wire the independent
onFrozenRecordingDestroyed observer to the ownership router's seal operation.

Sources:
- https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/nn-d3dcommon-id3ddestructionotifier
- https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/nf-d3dcommon-id3ddestructionotifier-registerdestructioncallback
- https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release

Added real WARP list destruction tests: private-data removal versus actual
object destruction, no notifier retention cycle, latest Reset identity, and
GPU-fence retirement remaining independent. Windows validation pending.

## Recording context and scene-release integration

Core43213581805 passed for18f288e, including actual WARP command-list destruction.
The native recording accessor now reads the exact original-thread context, queue,
device, SBT context, native fence and heap records independently of optional
reflection initialization. It rejects the wrong list/frame and faults without
publishing partial state. This accessor supplies borrowed identities; its caller
must retain COM objects, read real fence completion and validate actual bound
heaps. It never substitutes submission-thread TLS or certifies scene contents.

The submission router now atomically admits the whole owner with an acquired
scene token. It releases that token exactly once after actual consumer Execute
or proved recording cancellation/sealing. Scope cancellation alone cannot release
an unsubmitted producer. The release callback only opens the scene gate and
must not reenter the router. GPU owner retirement remains independent. Added
real-submit and successful-Reset coverage for both boundaries. Windows validation
pending for this checkpoint; live dispatch activation is still incomplete.

## Retained original heap and native-fence admission

Core43214779966 passed for1dfd5e9, including original recording-context parsing
and actual-submit/Reset-driven scene release. Full DLL validation was still in
progress when the next change was prepared.

The whole invocation now retains the original queue, native fence and actual
bound resource/sampler heaps via D3D12NativeGIHeapLease. Acquisition checks real
child devices, heap types, exact native record identities and actual fence
completion twice. A borrowed parser's completed field is deliberately ignored.
Both producer and consumer validate those saved records and native retirement
cut without reading the consumer thread's TLS. Native scene-content ownership
is still an independent requirement. Rearm clears the heap lease only after
whole-owner retirement. A WARP fixture exercises the real fence, including a
fabricated completed=0 with an already-completed epoch, which must reject.

Additional static inspection found direct pool-release callers at A6030,150AA0
and150CC0; provider update/build/finalize entry pointers appear in the audited
provider vtables. This inspection is not a complete indirect-call/source-write
closure and does not change scene_lease_proven=false.

## Hit-record geometry provenance

Core43215782524 passed for9393454, including the real native-fence/heap lease
and complete invocation compilation. Full DLL validation continues separately.

Extended the exact-renderer writer audit with eleven mutation-checked instruction
windows. Static-provider binding records keep index GPU addresses from the
original mesh NativeBuffer at mesh+160h, and attribute addresses from mesh+150h
plus mesh+178h. They are not owned solely by the RT BLAS allocation pool. The
static constructor stores parallel mesh pointers at provider+30h and64-byte
local binding rows at provider+20h. The skinned provider instead redirects
attribute addresses into its deformation allocation: provider+88h, allocation
+18h chunk, chunk GPU VA+10h plus allocation offset and per-mesh attribute span.
The same chunk NativeBuffer+8 feeds g_rwbVertexBuffer0/1 in rt_skinning.rfx before
A6BA6. Scene preparation copies provider virtual+50h rows into scene+80h, while
virtual+40h supplies the BLAS address for the separate scene+70h instance array.

The script passed original input and all15 writer/provenance mutations. This is
an ownership distinction, not a newly established explanation of the observed
noise. Enabling delayed traversal still needs the original static mesh lifetime
and write ordering covered alongside deformed/AS allocations and materials;
scene_lease_proven remains false. No user capture is requested for this static
reverse-engineering work.

## Game-DLL lifetime observer wiring

Added gi_frozen_lifetime.h after the native render/prepare/window adapters. The
existing opt-in frozen-render setup now installs the independent Execute,
successful Reset and actual destruction observers without replacing NRD or
legacy diagnostics. Before queue initialization the Execute observer returns
immediately; initialized queue ownership is published only after private fence
creation. It never waits, flushes or records extra GPU work at initialization.
Admission requires an already prepared complete owner, the current exact render
scope and an acquired scene token. Actual command recording and suppression
remain separate and are not enabled by this startup wiring. The callback router
is process-lifetime storage so D3D destruction callbacks cannot target a router
mutex already destroyed by C++ static teardown. Windows full-DLL compilation
pending for this wiring change.

Core job131174539253 explicitly logs PASS original native heap lease, PASS actual
D3D list destruction and PASS recorded GI heaps for9393454. These are component
results, not hardware DXR/gameplay or image/performance measurements.

Full DLL run43215782503/job131174609807 succeeded for9393454. Its packaged
artifact remains the R427v2 diagnostic configuration, not an enabled R428 visual
fix. The later lifetime wiring still requires its own full-DLL build.

## Original producer/queue identity and native mesh writes

Admission now also requires the prepared owner's exact original producer list,
its retained native queue and the current scope's frame/owner, before ownership
transfer or any replacement command. AcceptProducer repeats the original-list
identity check. These checks prevent an unrelated list/queue from reaching a
late submission quarantine after recording has already started.

Added audit-gi-native-mesh-writes.py and native-mesh-writes-r428.json. The exact
native buffer lock/unlock controls keep a NativeBuffer backpointer at+D8. Their
unlock write paths call native copy26990, whose direct and copy-list branches
load the existing destination resource from the NativeBuffer, then record
CopyBufferRegion. Seven mutated route windows reject. Hence unchanged wrapper,
resource and GPU address do not establish immutable source mesh bytes. This
does not establish that such writes overlap the user's GI pair.

Additional routes to inspect before designing a writer lease: native CPU-data
copy26840, whole-resource copy27CC0, DeferredBuffer copy17C80, and explicit
consume/copy transitions272D0/274A0. Blocking only the final Copy call can be
too late if its caller already recorded a transition. A blanket map lock could
also stall unrelated loading or deadlock a render dependency; no such lock has
been enabled. Existing NativeSceneWindow is not being promoted to a complete
source-mesh write lease on the strength of pointer identity.

Core43217871020 passed for7ac27c1; full43217870962/job131181610081 was still
running at this checkpoint. Live original-ray/GI6 command coordination and
complete source-content ownership remain unfinished. No new gameplay ZIP or
user test was delivered, and no image/FPS result is claimed.

## Model mesh source provenance and full DLL result

Full run43219696717/job131187822140 succeeded for145f5a3, including the new
lifetime observer wiring in the DLL. Core43219696698 also succeeded. The
packaging configuration remains R427v2 diagnostic; no enabled R428 gameplay
build, image result or FPS measurement follows from these passes.

Added audit-gi-native-mesh-sources.py and native-mesh-sources-r428.json, with13
instruction windows and two vtable windows; all15 mutated sites reject. Exact
vertex/index wrapper vtables are63B7B8/63B800 (not6337B8/633800). Both virtual+38
getters return the embedded NativeBuffer at wrapper+18. Mesh construction1BB010
calls those getters and stores the resulting pointers in mesh+150 (attributes)
and mesh+160 (indices), the fields read by the static RT provider. The wrappers
also expose upload, map and unmap operations at virtual+10/+18/+20.

Model loader1B9320 allocates and maps replacement wrappers at1BA573..1BA5DB,
rewrites mesh attribute references at1BA71E/1BA856, unmaps/releases the old
model+a8/+b0 wrappers at1BA9EB..1BAA39 and publishes replacement wrappers. This
connects the generic buffer APIs to real RT mesh source storage; it does not
establish that loading overlaps the user's GI calls, nor that every write caller
is covered by the existing RT preparation gate. Further work must cover this
loader's publication/lifetime contract. No global map lock or guessed source
lease has been enabled.

## Private hit-source snapshots (implementation started)

The local workspace had reverted to an older snapshot after interruption; the
554920d Git checkpoint was recovered without asking for another capture.

Instead of treating model reference counts or the RT preparation gate as proof
of immutable source bytes, add private copies of the index/attribute views read
by native hit shaders. Copy at the ORIGINAL ray use under its native source
state/order contract, then use rebased local-root addresses for BOTH ray groups.
The original TLAS/BLAS protection, material/texture/descriptor protection and
whole-invocation retirement requirements remain. This is not a new ray budget,
path tracing conversion, or RELAX blur adjustment.

Added NativeHitSnapshotPlan: validates full native declared bounds, rejects
ambiguous/overflowing/duplicate backing allocations, merges shared ranges,
preserves 256-byte address alignment, packs ranges into one private buffer and
changes only the two native GPU addresses. Native constants and two-row hit
record indexing are unchanged. Logical width is bounded; committed allocation
accounting is additionally enforced by D3D12NativeHitSnapshot. Larger storage
replacement budgets its temporary old+new allocation peak. Reuse rewrites fresh
bytes only after explicit whole-owner retirement; idle committed bytes remain
visible to the eventual shared budget.

CPU fixtures pass GCC warnings-as-errors and ASan/UBSan (LSan disabled because
the runner uses ptrace). They simulate source mutation and verify private views,
R16 tails, unchanged constants/rows, shared ranges, alias/overflow/budget rejects.
Windows WARP fixture added to core CI: actual CopyBufferRegion, native source
mutation, readback, retired reuse and debug-layer validation. GPU results pending.

Still not connected to the game: original native source-lease acquisition,
private-SBT rebasing/upload, invocation ownership and live producer/consumer
coordination. No source-content proof boolean is set merely by constructing a
plan. No FPS/image claim or new gameplay ZIP is made. The copy cost must be
measured; copied bytes and actual committed bytes need bounded reporting.

Core43231100293 passed for8e29d15, including the actual WARP private-source
mutation/reuse fixture. The preceding run failed at a warnings-as-errors test
compile (integer zero supplied to std::fill<byte>); explicitly byte-typed values
resolved it. There is no live DXR, image or performance result.

Private hit-table integration now reconstructs only the validated native hit
section in a private upload. It preserves the copied native raygen/miss region,
active even rows, shader identifiers and all material constants; the two local
GPU addresses are rebased. Native source SBT bytes are never overwritten. An
original writer pipeline/version mismatch or malformed reserved row rejects
before commands. Upload committed storage and replacement peaks are budgeted,
and old upload storage stays reserved during private-source growth.

The complete invocation now owns these source copies and the hit upload through
its normal retirement. Prepare requires the matching original writer table,
bindings and backing leases; both ray groups use the same private hit data.
Producer preflight includes the private-copy preflight. Once its first copy is
recorded, any later rejection is FailedAfterCommands and cannot cause a full
native-ray retry. Whole-owner rearm includes the private sources. Added a WARP
private-table test for unchanged native bytes, preserved raygen/miss, exact hit
rows, identity/budget rejects and retired upload reuse. Validation pending for
this integration delta. Live source acquisition and host dispatch coordination
remain unfinished; do not enable a guessed sceneAndDescriptorLease.

Core43231986207 passed private hit-table/source integration at05dc57f.
Added original-use source lease acquisition under the wrapper retirement lock:
complete native view bounds, retained backing, wrapper identity, read-state and
alias agreement are required; no raw wrapper escapes. New Windows fixture covers
partial/missing ranges, shorter native views, writable/disagreeing states, stale
addresses and retirement. This is lifetime acquisition, not queue/content proof.
Frozen-render startup now installs the creation/release registry without the
legacy AS/instance/barrier capture observers. Device creation proof is enabled
for this mode and legacy inventory is disabled. Release installation is
idempotent; without inventory it forwards after live retirement without counters.
Host coordinator and aggregate reservation/cache still remain unfinished.

Core43233212809 passed source acquisition and registry delta at4e64bb7.
Recovered pinned D3D cceb99cb again from existing file (no user re-upload).
Original-ray snapshot now has a distinct accessor that validates native frame,
counter, pipeline, complete layout and writer arguments on both sides of the
CPU copy and rechecks native identifiers. It does not replace the legacy job
consumer. Host still must acquire the joined scene window before invoking it.
New exact-binary audit confirms358E4 DispatchRays precedes35944 job arguments
and359D9 runJob: the pre-schedule writerReady callback is too late for phase A.
Five instruction windows and five mutations passed. No GPU-completion or
scene-content proof is inferred from this sequencing.

Added whole-invocation CPU allocation preflight before private GPU allocation.
It accounts all nine committed buffers, retained oversized storage and old+new
storage when growing a retired allocation. RetainedAllocationBytes remains
available after rearm or partial failure for eventual active+idle accounting.
Added WARP exact/insufficient budget and overflow checks with no private GPU
allocations during preflight. Windows validation pending for these changes.
This is per-owner preflight; host aggregate reservation/cache is still required.

Core43234529975 passed at1d5c7d4, including nine-buffer preflight.
Added a shared atomic allocation reservation owned by the whole invocation.
Prepare now requires a host-supplied shared budget and reserves its complete
peak before any GPU allocation. Success/partial failure settles to actual
retained committed storage; the reservation remains across retirement/rearm and
is released only after member GPU resources are destroyed. Thus active,
preparing and idle owners share one ceiling. Unexpected underestimation closes
future admission. Local GCC warnings-as-errors and eight-thread contention test
passed. Windows validation pending; actual retired-owner cache/host coordinator
still need wiring. This change makes a missing shared budget fail before work.

Core43235990233 passed shared budget at45caf693. The preceding core failure was
CMD/Powershell syntax mixed in the generated test batch, corrected without a
rendering-code change. Full DLL validation remains pending.

Existing R306 writer captures (four snapshots, 5,690–13,293 bindings) imply a
59.62–180.47 MiB union LOWER BOUND for copied views per invocation. Reproducible
estimate-gi-hit-copy-bytes.py and hit-copy-lower-bound-r428.json retain exact input
hashes. This is not a measured GPU duration, actual packed size or copy command
count: those captures lack allocation identities and packing can include gaps.
Avoid inferring current-scene FPS cost from this older data.

Private-source barriers are now prebuilt during Prepare and batched into at
most two ResourceBarrier calls per recording, instead of separate transitions
around every source copy. Individual CopyBufferRegion operations remain. WARP
fixture expanded to distinct source allocations and both initial/read-state and
retired COMMON-state batch paths. Windows validation pending for this delta.
