M12R57 - snapshot PT inputs before scheduling the native writer

R56 results show 13 handoff_writer_pending fallbacks after startup. PT checked
readiness while the native worker was still running and retained RT that frame.
Metrics pooling worked (3 created), with preparation about 3.8-3.9 ms.

Publish the CPU input snapshot after exact scheduled-job validation, before
runJob starts the worker. PT reconstructs its table from identifiers and binding
rows; it does not read the GPU shader table produced by that worker. Worker
completion now only retires its generation. Scene submission, allocation identity,
fresh view and final admission checks remain required. No waiting or stale-frame
reuse is added. Upload and metrics pools, shaders and settings are unchanged.

The bridge regression requires readiness before Begin, rejects mismatched jobs,
and checks duplicate/stale completion cannot publish again.

Close Control and extract beside dxgi.dll. Keep PT enabled and test DLAA/Quality
in the same scene, watching for flicker over several minutes. Close and run
Collect-PT-Primary-M12R57.cmd. PT selection remains persistent and restart-only.
In-game flicker and performance still require confirmation.
