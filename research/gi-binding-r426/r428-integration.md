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
