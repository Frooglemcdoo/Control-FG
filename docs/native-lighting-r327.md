# R327 — persistent validated conversions and retired table storage

## Evidence and correction

R326 PID 80200 capture (20261104-152655): all nine indexed entries verified.
259 admissions; inclusive CPU averages: owned preparation 5.052 ms, scene
admission 20.400 ms, mapping 12.361 ms, table commit 4.002 ms. R325 was
4.295 / 16.599 / 9.795 / 3.447 ms respectively. The recordings are not a
controlled benchmark and these nested scopes must not be added as frame time.
The user reports unchanged poor FPS. There is no demonstrated R326 improvement.

R326 increased logged reuse (one scene: 3543 to 9221 of 11344 geometry visits),
but its map was destroyed after each admission. Its successful-match diagnostic
census change affected only the first three diagnostic admissions in this host.
Native GI and the private queue admission hold remain material unresolved costs.

## Changes

- Keep bounded CPU conversion records across admissions. Each entry includes
  exact BLAS allocation/build/geometry identity, the complete native row,
  original geometry witnesses, frozen range layout, and weak backing-owner
  identity. Pipeline or export changes clear the cache. The first hit per locked admission checks the
  current unique original owners and exact position allocation extent before
  fresh leases are acquired. Overlaps, retired/replaced tokens, different
  materials, frozen-storage changes and altered bounds cannot use a stale entry.
  No scene cut, resource lease, GPU sample, or traversal permission is cached.
- Keep a compact transaction-local hash index for repeated instances. This
  avoids probing the larger persistent cache and reacquiring weak owners on
  every visit. Both indexes compare their complete keys; hash collisions cannot
  authorize reuse. An initial all-visits persistent-cache prototype regressed
  the synthetic benchmark and was revised before delivery.
- Rebind CPU epoch metadata to the current admission. Current queue order,
  complete traversal closure, geometry metadata and row conflict checks remain.
- Compare the captured native table directly against bindings, including active
  masks and all reserved/padding bytes. This removes allocation, construction
  and zero-fill of a redundant full-size native table at each admission.
- Reuse retired shader-table upload buffers through the existing exact-device,
  exact-size, heap-type pool. The existing pass/recording/flight owner keeps them
  unavailable until its lifetime ends. Reserve clears every byte; commit writes
  the entire validated table before the gate is armed. In-flight tables never
  enter the pool. Total shared idle storage remains capped at 128 MiB/128 buffers.
- Sample persistent-hit counts after warmup. Separate owner-lock wait, mapping
  work, lease assembly and deferred-resolution CPU time in the automatic logs.

The persistent cache has 16384 direct-mapped slots; collisions only cause a
fully validated miss. It holds weak frozen owners, so it cannot retain obsolete
geometry GPU storage. Cache bytes are bounded independently of scene duration.

## Limits and next dependency

This is the first persistent-record step, not the complete submission redesign.
Final scene admission and complete GPU table commit still occur at submission.
The private queue wait remains necessary until preparation can safely finish
before submission. It must not simply be deleted: that would permit the GPU to
read unfinished controls/tables. Native GI still executes for fallback; no rays,
lighting resolution or shading quality have been reduced. No in-game FPS gain
is claimed from compilation or synthetic tests.

NVIDIA recommends persistent geometry/material data and incremental shader
tables: https://developer.nvidia.com/blog/practical-tips-for-optimizing-ray-tracing/
Microsoft describes fence-safe storage reuse:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/fence-based-resource-management

## Validation and test

CPU tests cover warm-cache output parity, new epochs, overlapping owners,
owner removal, changed frozen owners/ranges, typed/raw index bounds, both index
widths, material/geometry distinctions, row conflicts and competing queue writes.
The native-table comparator rejects a mutation of every byte and active flag.
WARP scratch tests exercise retired upload reuse and full overwrite/readback.
Windows foundation, producer and proxy workflows are required before delivery.

`tools/benchmark-gi-scene-mapping.cpp` supplies a reproducible CPU-only fixture:
11344 instances, 1950 distinct material rows, one shared immutable BLAS, frozen
geometry, 5 warmups and 50 timed preparations for each index width. GCC -O2,
four interleaved pairs on this host: R326 median 9.776 ms (R16), 8.630 ms (R32);
revised R327 median 6.463 ms (R16), 6.251 ms (R32). This intentionally isolates
repeated mapping; it does not reproduce the game's full BLAS distribution,
locking, D3D12 calls, private queue hold, native GI or GPU rendering. Do not
translate these measurements into an FPS estimate. ASan/UBSan passed with leak
detection disabled because this executor cannot enumerate threads for LSan.


Install all contents beside Control_DX12.exe preserving folders. With the game
closed run ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd. Keep RR, RT indirect
lighting and GI RAW BYPASS on, at the same resolution and ray count. Compare in
the same area, switch Native then Owned, and exit normally. Send the automatic
diagnostics ZIP; no separate capture key or standalone test is required.
