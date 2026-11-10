# R411: stop optimizing local hotspots and isolate measurement overhead

R410 capture probe-20261110-123935-980-16076.log:
3583 refreshes, 3564 full, zero partial, 19 skipped; 259271150 requested
descriptors versus 256535745 copied (1.055% reduction). 221689160 publications
each passed through the tracking SRW lock. Between the 1424 and 3583 reports,
2159 refreshes were all full: no descriptor-copy savings in that interval.

R409's 84% refill share is of sampled allocator wait/hold/release, not total
frame time. Its two lock-call wrappers run on every allocator call, including
unsampled calls. R410 removes those wrappers but introduces a different
all-publication hook, so similar observed losses do not prove one shared cause.
Timing-log write cost does not measure all instrumentation overhead.

Warm object counts also differ: R408 5735.93 objects/preparation loop, R409
7323.56, R410 7769.21. These are different workload populations, so differences
in aggregate job times cannot isolate a code regression. Earlier scheduler
audit establishes waitJob may execute other jobs; inclusive wait is not idle.

## Controlled comparison

Compile the same source twice, differing only in CONTROL_RT_DIAGNOSTICS=0/1.
Default root DLL is clean. R410 tracking is never installed in either binary.
Preserve all owned rendering functions, native updates, descriptor batching,
ray counts, TLAS flags and lifetime behavior. Clean executes no RT stats
aggregation, probe scope, phase clock or frame interval recording. The record
phase callback is null. Diagnostic restores the R409 measurement path.
No live hotpatch switch: select the DLL only with the game closed.

Startup logs identify the mode and install results. The clean binary cannot
provide its own continuous frame measurements without reintroducing an
observer; compare with the same existing FPS counter. Clean/diagnostic/clean
controls warmup/order better than unrelated runs. Existing general mod features
and user-triggered capture facilities are unchanged, not declared overhead-free.

## Research basis and acceptance

PIX critical path analysis:
https://devblogs.microsoft.com/pix/critical-path-analysis-in-timing-captures/
Profiling options and overhead:
https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/timing-captures/pix-timing-captures
Frequent short-event observer overhead:
https://developer.nvidia.com/blog/understanding-the-visualization-of-overhead-and-latency-in-nsight-systems/
Persistent descriptor guidance (architectural, not proof for our implementation):
https://developer.nvidia.com/blog/advanced-api-performance-descriptors/

Do not resume allocator/descriptor optimization until the clean baseline and
critical dependency are established. A clean-vs-R410 gain alone is ambiguous.
A matched clean-vs-diagnostic gain attributes the RT profiling contribution,
but does not identify which individual probe dominates. A negligible difference
requires investigating another cause, not adding another small optimization.
No performance claim from successful correctness tests or reduced call counts.

Build validation checks absence/presence of all RT periodic profiling report
markers, identical ownership install markers, disabled R410 tracking, and
hash-verified reversible DLL selection. Existing native execution and GPU
correctness tests remain required.
