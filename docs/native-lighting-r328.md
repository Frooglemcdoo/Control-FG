# R328 — reuse admission checks while their inputs are immutable

## Measured R327 evidence

Capture 20261104-162503, PID 49492: all 15 indexed files passed SHA-256
verification. 366 admission samples averaged 15.515 ms inclusive CPU time.
Mapping work averaged 7.663 ms, owner leases 0.900 ms, scene selection 2.893 ms,
and table commit 3.490 ms. Owner lock wait rounded to 0.000 ms (maximum 0.001).
The user reports that FPS remains poor. Nested scopes are not additive frame
timings; elapsed deferred holds are not measured GPU idle time.

R327 persistent conversion hits are present after warmup. That cache does not
eliminate repeated scene-order selections in catalog selection, complete
traversal closure, and geometry mapping, or repeated backing-allocation range
queries for instanced geometry at table commit.

## Changes

- Reuse successful ordered AS selections at the same cut, allocation token,
  address and observation generation. The bounded 16384-slot CPU cache stores
  revision IDs only. Every metadata mutator invalidates it, including failed
  submits/signals/waits, fence completion, cut creation, explicit invalidation,
  compaction and checkpoints. Later unordered writes and source-dependency
  races must still invalidate earlier successful selections. Failures are not
  cached. Full key comparisons make collisions misses, never authorization.
- Reuse exact backing-range containment results inside each immutable table
  commit batch. All resource leases are still acquired, all ranges indexed,
  and geometry records validated. The 4096-slot cache preserves missing-range,
  overlap/alias, overflow and endpoint rejection. It cannot survive a new batch.
- Retain R327 conversion reuse, fresh owner leases, full shader-table writes,
  queue admission gates and GPU completion lifetimes.

No ray count, lighting resolution, shader or image-quality setting changes.
This is a bounded CPU admission optimization, not a completed asynchronous
submission redesign. Native fallback GI and the private queue hold remain.
Do not remove the hold while tables and control data are still unfinished.

## Validation

Portable tests cover ordered metadata, deferred scene catalog selection,
complete scene packets, table validation, warm-cache invalidation by competing
writes and source races, failed signals, explicit invalidation and pending
fence completion. Randomized backing-range tests compare both first and cached
queries against exhaustive containment with overlapping allocations.
ASan/UBSan runs of the modified ordered-metadata and producer-table tests pass
(leak detection disabled because this environment cannot enumerate threads).

`tools/benchmark-gi-admission-queries.cpp` models four selection passes over
2048 distinct AS versions at a new cut each trial, and 11344 three-view queries
over 2048 backing allocations. Initial paired optimized Linux results:
R327 2.894 ms selection / 2.874 ms ranges; R328 2.240 / 0.951 ms.
These are isolated CPU workloads, not in-game frame timings or an FPS claim.

## In-game check

Extract beside Control_DX12.exe, preserving the package folders. With the game
closed run `ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd`. Keep RT indirect
lighting and RR on, select GI RAW BYPASS, and compare NATIVE then OWNED in the
same location at the same ray count and resolution. Exit normally to collect
the automatic capture ZIP. No capture hotkey or standalone test is required.
