# R218 native CPU timing test

This build adds targeted native CPU measurements to the accepted R217 visual baseline. It does not increase worker count or move rendering work between threads. No performance gain is claimed.

Install the package over the existing test installation, as with R217. Keep your normal Native Blend/RR configuration, GI Clamp Only, Smooth On and GI View Normal. Map Guard stays off; restored angled reflections and the accepted ray projection remain intact.

Play the area with low GPU utilization for 30–60 seconds without screen recording. Exit the game, run `Collect-ControlFG-Compact-Logs.cmd`, and send its ZIP. No video, WPR capture, F9 suite or manual lighting capture is needed. Measurements go into the normal probe log automatically.

## What is measured

- Material-table update at renderer call `0x12491D`.
- RT preparation job through execute slot `0x6281F0`.
- RT command recording job through execute slot `0x6281D8`.
- RT preparation join at call `0x12404D`.

The join executes the original native scheduler. Its elapsed time includes useful jobs the waiting thread may execute; it is not an idle-time metric. Job execution can overlap a join, so stage totals must not be added as disjoint frame times. The preparation/recording measurements cover their native job callbacks, not any separate serial fallback. A zero callback count is reported rather than inferred as zero work.

One invocation in 64 per stage per thread is timed with QPC and thread-cycle counters. Counters and thread IDs are bounded to 64 observed threads. Cumulative summaries are written approximately every five seconds, at a 60-Present boundary. `tid_calls` maps thread IDs to observed callback counts, not cores or simultaneous workers. Sampling is periodic and can miss spikes; `sampled_max_ms` is the largest sampled duration, not an all-frame maximum. Cycles are raw CPU cycle counts, not CPU milliseconds, and include instrumentation overhead. Failed cycle queries are excluded and counted separately through `cycle_samples`. Wall samples and calls are cumulative with slightly different read instants. Busy snapshots are skipped and reported. No GPU timers, stack capture, GPU waits or new worker jobs are introduced.

`NATIVE_CPU_R218_INSTALL ready=1` and `NATIVE_CPU_R218 enabled=1` confirm installation. If an expected call/slot differs, the timing hooks fail closed and the existing graphics configuration continues. Applied hooks are rolled back on partial installation failure. All hook original targets and relay storage stay valid for process lifetime, including rollback failure.

For an instrumentation-off comparison, set process environment variable `CONTROLFG_NATIVE_CPU_TIMING=0` before launching Control. Restart is required; this is not a live patch toggle. Normal launches default to timing enabled in R218.

Validation: exact byte/slot checks against the supplied renderer, concurrent counter tests, Windows executable relay/rollback and argument/LastError/exception tests, followed by the existing shader and proxy build checks. Actual game timing and stability still require running this package.
