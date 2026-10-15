M12R58 - bundled writer timing and scene preparation performance fixes

R56 results show 13 handoff_writer_pending fallbacks after startup. PT checked
readiness while the native worker was still running and retained RT that frame.
Metrics pooling worked (3 created), with preparation about 3.8-3.9 ms.

Publish the CPU input snapshot after exact scheduled-job validation, before
runJob starts the worker. PT reconstructs its table from identifiers and binding
rows; it does not read the GPU shader table produced by that worker. Worker
completion now only retires its generation. Scene submission, allocation identity,
fresh view and final admission checks remain required. No waiting or stale-frame
reuse is added. Upload and metrics pools, shaders and settings are unchanged.

Scene construction now shares immutable geometry vectors from the exact resolved
catalog snapshot, including compacted source ownership. It avoids per-BLAS vector
allocation/copy in preview and final admission and makes proof topology copies
share those same immutable rows. Identical shared geometry can compare by identity;
distinct snapshots still receive the original field comparison. All queue ordering,
version, allocation, row and shader validation remains active.
A local synthetic 1000-BLAS/100000-geometry scene benchmark (250 constructions)
measured 0.945 ms before and 0.322 ms after. This is not an in-game FPS prediction.
Tests cover replacement versions, compacted sources, catalog destruction, last
scene release, null shared input and equal/distinct immutable geometry.

The bridge regression requires readiness before Begin, rejects mismatched jobs,
and checks duplicate/stale completion cannot publish again.

Close Control and extract beside dxgi.dll. Keep PT enabled and test DLAA/Quality
in the same scene, watching for flicker over several minutes. Close and run
Collect-PT-Primary-M12R58.cmd. PT selection remains persistent and restart-only.
In-game flicker and performance still require confirmation.
