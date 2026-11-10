# R407: reserve disjoint compute tables within a recording job

## Evidence and scope

R406 probe-20261110-003425-410-68444.log installed every active hook, no errors.
Excluding the first 16384 applications, 704 detailed samples accumulated
19152 us; shaders accounted for 17992 us (93.9432%, 25.5568 us/application).
Ordinary samples are a separate cohort (4928 calls, 125592 us). No FPS claim.

D3D setShaders 2F050 invokes 1BD50 before comparing cached shader pointers.
1BD50 always reserves 49 CBV/SRV/UAV slots via 1AA00, reserves samplers via
1ACF0, and conditionally initializes all 49 descriptors. 1AA00 takes the shared
manager lock; it checks prefix generation and space, handles heap creation or
completed-fence reuse, copies the bindless prefix when required, advances the
cursor, stamps retirement, and returns GPU/CPU handles plus heap owner.
Thus the expensive measured stage includes more than shader-pointer comparison.
We have not measured which internal part dominates. R407 removes repeated
1AA00 calls on eligible hits while retaining the rest of 1BD50.

## Native allocation contract

Build-locked D3D SHA256:
cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5.
Guard exact bytes 1BD50..1BDEF and 1AA00..1ACEA. Redirect only the E8 at 1BD85,
original target 1AA00. Outside owned recording job + known skinning application
scope, forward the original call unchanged. No global allocator entry patch.

Manager pointer: D3D+111BB8; generation manager+8; increment D3D+111BC0.
Frame object D3D+136D28, timeline frame+88, retirement stamp timeline+0.
Native allocation stores this stamp in provider+20 and prefix generation in
provider+30. CPU/GPU handles and provider are returned to context+40/+30/+50.
Native dispatch setup 2D150 reads the context's +50/+58 heap owners; it does
not rely on the manager's current heap for the leased table's binding.

A stack-local lease lives only for one RTOwnedRecord invocation, with recursive
scope restoration. One native reservation requests 49*16 slots. The first
and each subsequent take publish a distinct 49-slot slice. The native cursor
already excludes the whole reservation from unrelated allocations. There is
no heap or descriptor-content reuse introduced. Up to 15 unused tables per
reservation are abandoned; this can increase heap consumption and is a tradeoff.

Hits require identical manager, native thread context, timeline identity,
nonzero epoch, prefix generation and increment. The returned owner must still
carry the matching epoch and prefix generation. Identity is checked before
and after fresh reservations; changed identity prevents caching the remainder.
Heap/fence lifetime stays with the native reservation. A pending recording job
must complete before its command work can be submitted/retired; no lease is
retained for a later job. Changed timeline/epoch forces a fresh reservation.
The lease does not suppress any sampler reservation, null initialization,
constant update, live descriptor write, resource transition, compute dispatch,
AS build or retirement operation. The existing glass/GI observer fallback
avoids entering the owned application scope during those captures.

## Validation

Portable test: 130184 tables across 2048 jobs; disjoint contents, interleaved
native reservations, job reset, epoch/version/context/timeline changes, owner
stamp invalidation, unsupported sizes, zero epoch and arithmetic overflow.

Native 1BD50 machine-code oracle: 4096 cases comparing GPU/CPU outputs, owner
fields, both sampler modes and complete 49-descriptor initialization calls.
Five-argument reservation forwarding is exercised through the relocated body.
The original R405/R406 native binding oracle remains unchanged and passing.

WARP test extends descriptor-copy validation with 32 distinct leased tables,
recording all dispatches before submission. Each table has different input
bindings and must retain its own GPU output. Two reservations serve all 32;
existing wrong-source negative control remains present. This is correctness
validation, not an RTX benchmark or proof of performance improvement.

Counters report native reservation invocations versus leased tables. The
sampled R406 shader phase remains in R407 to measure whether this actually
reduces the identified cost. Sampler locking and full-table initialization
are still possible remaining costs; no percentage speedup is predicted.
