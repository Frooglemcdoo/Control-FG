M12R59 - bounded allocation lookup cache and scene preparation breakdown

R58 user result: flicker resolved, approximately 1-2 FPS gain. Quality preparation
averaged 3.541 ms, with 3.317 ms inside scene preparation. The aggregate does not
identify how much belongs to scene preview, validation or resource retention.

Replace the per-query tree cache with 16384 fixed direct-mapped entries (512 KiB,
allocated on first lookup). Exact address, byte count and allocation generation
must match. Hash collisions evict entries and invoke the original interval scan;
they cannot produce a match. Every Insert/Remove invalidates all prior entries
by generation. Clear and generation overflow also reset cache validity. Negative
and ambiguous results are cached under the same exact checks. No scene checks
are skipped. Cache entries are not resource ownership proofs.

Local synthetic benchmark: 4096 allocations, 16384 queries/frame, 400 frames.
Warm lookup 0.407 -> 0.344 ms; invalidated each frame 1.38-1.43 -> 0.63-0.64 ms.
These are CPU microbenchmarks, not measured in-game savings or FPS predictions.
Differential regression compares 200000 generated queries plus repeat hits with
an independent uncached scan across overlapping ranges and allocation changes.

120-frame PT_PRIMARY_SCALING windows now include handoff lock wait, scene preview,
owner lock wait, plan validation, resource retention and proof construction.
Original CPU/GPU metrics and pool counters remain. R58 flicker fix is retained.

Close Control and extract beside dxgi.dll. Test PT on with DLAA/Quality in the
same scene, approximately 30 seconds each. No enable script needed. Close the
game and run Collect-PT-Primary-M12R59.cmd. Send the ZIP and average FPS.
