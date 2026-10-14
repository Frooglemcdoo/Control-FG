# R45 descriptor copy counter contention

R44 CPU.verbose capture: process 48768; 2026-10-14 03:02:35 UTC start;
10 seconds recording, analysis excludes first/last second. WPR reports zero
lost events. Loaded DLL SHA256 e89e926b413cae4897a770222708bb900ab07bf1ee9391147156056a0d96c9f6.
R44 bypass separately confirmed capture=0; user reports Quality about 140 FPS
versus 90-95 normal capture with PT off, DLAA about 80-85 in either mode.

32279 game CPU samples in the 8-second analysis window. 1908 instruction
samples map inside TryParallelCopy, including 650 at function offset 0x44f,
362 at 0x454 and 70 at 0x45a. Exact R44 local PDB function RVA 0xcfc0;
these locations correspond to the shared mirrored-descriptor increment,
global version fetch-add and the instruction immediately after that fetch-add.
These are sampled instruction locations (including possible sampling skid),
not exact elapsed time or a whole-frame CPU accounting. The CPU-only profile
contains no DXGI Present events; no Present FPS is derived from it.
Other remaining costs include descriptor locking and shader byte classification.

R45 replaces the global version sequence with monotonically increasing
per-slot generations. Snapshot comparisons already use the same retained heap
and slot; no ordering between slots is required. Both exclusive Mark and shared
copy call Advance. Generations never reset on invalidation; saturation sets
the global exhausted flag, which all snapshot readers already reject.
Application serialization of conflicting native descriptor accesses remains
required; shared gate excludes snapshots and exclusive observers. Different
slots can advance independently without atomic increments.

Parallel copy statistics use 128 alignas(64) atomic counters. Each thread
selects a shard once; collisions remain correct. Statistics acquires exclusive
gate and sums all shards into the existing single-copy and descriptor totals.
One relaxed atomic replaces three globally shared operations per copy. No
native API forwarding, sparse metadata, source tokens, AS identity, geometry
ownership, queue synchronization, or shader behavior is changed.

Tests retain 16000 adjacent concurrent native/mirror copies and WARP readback,
add exact aggregate counter deltas and alternating exclusive/shared writes to
one slot with unchanged neighboring generations and stale-snapshot rejection.
In-game performance improvement remains to be measured.
