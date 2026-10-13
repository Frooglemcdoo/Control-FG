# M12R4 evidence and validation

R3 log: 3294 geometry-copy failures, zero copy allocations, zero primary PT
submission; first catalog failure is recording_rows_limit. FPS restoration was
fallback, not evidence of successful tracing.

Native Steam DLL hash remains cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5.
Creation at 0x26360 writes default resource state to NativeBuffer+0x20. The
native copy routine at 0x26ad6 reads this field to restore its default state.
The existing entrance overview capture shows compute list 5832, GlobalId 1140,
transitioning position resources 3243/3568/3163/3426 from UAV to NON_PIXEL_SHADER_RESOURCE
immediately before BLAS updates at 1141 onward. The wrapper field alone is
therefore insufficient for the geometry-copy barrier's StateBefore.

M12R4 observes actual command-list buffer transitions, resets them with the
recording, retains resource identity, rejects unfinished split transitions,
and excludes injected copies from native observation. WARP tests exercise
COMMON, explicit read, and temporary read with a differing UAV wrapper default;
the temporary-read fixture uses a compute queue, matching the captured path.

Scene history now compacts immutable metadata without requiring simultaneous
queue idleness. Vector-clock timestamps preserve existing cross-queue ordering.
Only subsequent appends gain CPU-observed completion edges. Active dispatch and
unresolved compaction cuts remain roots. Per-key/per-lane writes immediately
before AND after each root are retained, as are latest writes and tombstones.
Resolved compacted geometry shares its immutable copy storage. Recording,
submission and rendering leases retain actual GPU memory independently.

Portable checks compare compacted and uncompacted scene decisions, keep old cuts
unordered after later completion, retain compacted geometry across 20000 updates,
and keep metadata bounded while GPU completion lags the newest submission.
Existing tests check late signals, reused allocations, rejected geometry, and
protected P2/G3 rendering source. Windows CI checks and the in-game outcome must
be reported separately: this evidence does not establish RTX 5090 PT coverage.
