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
traffic and these owned failure paths. Windows validation pending for this delta.

These corrections do not enable the frozen GI dispatch. The previous diagnostic
package remains a diagnostic, even if the full packaging workflow succeeds.
Do not label its automatically generated artifact as an R428 visual fix.
