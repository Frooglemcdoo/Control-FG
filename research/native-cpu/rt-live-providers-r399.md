# R399: remove RT provider getter dispatch with live reads

R398 visual result was normal. Existing log probe-20261109-161058-492-53312.log:
6,656 selection queries, total 1,635,268 us (mean 245.683 us), zero fallback;
6,656 preparation loops, total 13,847,782 us (mean 2,080.496 us), 49,850,007
selected objects and 2,376,455 dynamic updates. Assembly reports have different
flush boundaries. No FPS improvement inferred from these figures.

Renderer SHA256 ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433.
RTTI identifies RaytracingStaticMesh vtable 6332F0 and RaytracingSkinnedMesh 627E38.
Slot 48 returns int32 count (+28 static, +30 skinned). Slot 50 returns descriptors
(static +20 pointer/index*64; skinned +18 pointer/index*80+16). Slot 60 returns
mask (+60 static, +54 skinned). Slot 40 returns BLAS address: static signed state
+AC >=3 selects allocation +88/offset +90, otherwise +68/+70; skinned follows
state +88 to allocation +28/offset +30. Both add sign-extended int32 offset to
allocation GPUVA +10 with unsigned 64-bit wrap. Exact bytes are embedded for
installation checks and execution in the parity oracle.

Bind only after renderer hash, byte and vtable-slot verification. Per assembly,
check current vtable identity and all four getter slots before specialization.
Other providers or changed slots stay virtual. Pure getter reads occur at the
same assembly stages as before. Readiness and dynamic update callbacks remain
native and execute before assembly; no across-frame state cache or snapshot.
Vtable/code stability during a single assembly call is required, as in existing
hooks. In-place code patching after installation is not supported.

Avoided calls per recognized provider = 3 + positive geometry count. New counters
measure coverage; this is not an expected FPS number. TLS telemetry batches as
before. The generic path retains the original 4,096 oracle comparisons. A further
8,192 comparisons execute the exact native getters inside the original assembler
against specialized output, mutating the same provider across iterations and
covering signed branch/offset edges, zero/64 records, masks, live addresses, NaN
transform bits, growth and virtual fallback. Native readiness/updates are outside
this leaf test; existing outer-loop and selection tests remain required.

Next evidence: normal gameplay log with coverage and same-scope outer-loop timing.
No new standalone capture tool or ray-count change. No FPS gain established.
