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
Windows test is included in gi-replacement-core; pending CI. It exercises the
native layout reader and inaccessible-pointer rejection in addition to the
portable wrong-context/retirement cases.

Still required before a user visual build: native scene scope integration,
exact hit/output/SBT acquisition, bounded reusable invocation storage, actual
producer/consumer queue/reset callbacks and completion/failure publication.
No new noise or FPS result is claimed.
