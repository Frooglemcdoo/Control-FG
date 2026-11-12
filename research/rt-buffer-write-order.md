# Ordered buffer mutation evidence

`src/gi_reuse/ordered_buffer_writes.h` adds buffer mutation stamps using the
existing `PTSceneOrder::Graph`. It does not add another queue scheduler, fence
implementation or resource-retirement ledger. COPY, direct and compute queues
use the same graph rules and exact queue/fence lifetime identities.

The host registers an allocation lifetime and submits immutable write recordings
in actual Execute/list order. Whole-resource copies are explicit; zero-byte ranged
copies are not mistaken for whole copies. Every nonempty write changes the entire
allocation's stamp conservatively. Replay is another write; an unsubmitted/reset
recording must never be fed to Submit. A read cut is rejected when a competing
writer is unordered, even when that writer was observed after the cut. Joining
two racing writers with waits does not invent an order between the writes.

CPU writes invalidate reuse for the allocation's remaining lifetime. Neither
Unmap nor a subsequent GPU copy clears that rejection: there is no established
exclusive CPU-write interval yet. Retiring and re-registering the same VA requires
a new lifetime token. The host still needs physical alias exclusion; a new token
alone does not supply it.

Stamps contain no byte snapshot and prove no GPU completion, resource state or
future read protection. `Unchanged` compares a prior mutation stamp at a new
ordered cut; it does not authorize a dispatch by itself. CPU-write notification
must precede exposure of writable storage, and cannot repair a read already
executing on the GPU. Recording ownership and storage retention stay in the
existing lifetime machinery. Allocation exceptions must invalidate admission at
the host boundary, as with the existing ordered AS metadata adapter.

The model is bounded at 65,536 allocation lifetimes and fails closed on exhaustion
or inconsistent identities. It reuses nonblocking graph compaction and remaps
external cuts. Registration history is not age-evicted. This bounded evidence
model is not yet an enabled long-running game cache.

## Validation and integration status

`tools/test-rt-buffer-write-order.cpp` covers cross-queue dependencies, unordered
late writes, the two-writer race, whole/partial/empty writes, replay/discard,
CPU invalidation, VA reuse under a new token, malformed recordings, failed signals,
pending signals and 1,000 compactions with retained cuts. GCC warnings-as-errors
and ASan/UBSan pass (leak detection disabled for this executor). The target is
included in the existing PTLAS CMake validation suite for Windows compilation.

No production caller is connected to this model yet. The prior change exposed
ordinary copies at the live callback boundary; connecting retained recordings,
COPY-queue submissions and CPU-write entry interception is still required.
In particular, do not replace the existing `GINativeSceneQueues` COPY-queue skip
with a claim of coverage without also observing all relevant queue operations
and retaining the actual resources. Existing frozen snapshots remain required.
No GPU copy is removed and no native producer is suppressed by this change.

R433 remains held. These tests run without a new user capture and do not establish
an RTX queued-update result, a gameplay performance gain or a speckle fix.
