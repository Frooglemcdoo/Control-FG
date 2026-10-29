# R219: native RT record-append fast path

R219 makes one CPU optimization inside the existing ray-tracing preparation job. When its geometry/instance array has spare capacity, the two audited append callsites use a smaller path to write the same 64-byte record, increment the same count, preserve capacity/ownership flags, and return the same record address. Capacity growth and overlapping source/destination/header cases delegate to the original native function. Allocation, object lifetime, job ordering, material IDs, LOD selection, resource publication, and GPU commands remain native.

The R218 run measured sampled averages of 3.51 ms for RT preparation, 1.69 ms for material update, 1.25 ms for RT recording and 2.35 ms for the preparation join during a 90-second post-startup window. These overlapping durations are not additive. R219 targets a repeated append operation within preparation; it does not eliminate the entire preparation cost. No in-game FPS improvement is confirmed yet.

## Test

Install over R218 and launch normally. The fast path and the existing low-rate CPU timing are enabled by default. Keep your usual Native Blend/RR settings, GI Clamp Only / Smooth On / Normal view, and Map Guard off. Play the same area for 30–60 seconds without screen recording, then exit and run `Collect-ControlFG-Compact-Logs.cmd`. Send its ZIP and whether FPS or smoothness changed. R217 visual fixes and restored angled reflections remain intact.

For a direct comparison, exit Control and run `Start-RT-Append-Native.cmd` from the game folder. This uses the original native append routine with the same call hooks and counters. Exit and use `Start-RT-Append-Fast.cmd` to test the new path. Both scripts set a process-local environment variable; they do not modify saved graphics settings or global environment. Normal Steam/Epic/GOG launches default to the fast path unless their parent environment sets `CONTROLFG_RT_APPEND_FAST=0`.

## Evidence and limits

The supplied renderer's original non-growing append instructions were executed in an isolated x64 CPU test. Candidate and original produced identical records and return addresses for 4,096 unaligned appends. Ten alternating-order timing pairs favored the candidate in that host/compiler microbenchmark; this is not a Windows-game FPS measurement. Portable randomized tests also cover the four ownership-flag combinations, growth boundaries, aliases, record bytes and return addresses. Windows tests exercise forwarding/fallback and page-protected copy boundaries. The existing graphics validation suite runs on the same source.

`NATIVE_RT_APPEND_R219_INSTALL ready=1` confirms hook installation. `NATIVE_RT_APPEND_R219 installed=1 enabled=1` confirms the requested path. Periodic `NATIVE_RT_APPEND_R219_STATUS` records show cumulative fast/native append counts published at completion of native preparation job callbacks. Counts exclude work not completed through those callbacks, including a separate serial fallback. No timing or log calls occur within the append loop; its diagnostic counters are thread-local and published once per job.

The R218 timing log format remains unchanged. It samples one call in 64 per stage per thread and reports cumulative values. The preparation join includes useful scheduler work; it is not pure idle time. Inspect interval differences after loading rather than using lifetime means that include first-use spikes. The optional native comparison includes the wrapper/counter overhead, so it is an instrumented comparison, not byte-for-byte R218 execution.

The optimization changes two verified direct callsites, only at startup after the existing renderer image hash gate. A byte/target mismatch declines installation. Partial installation rolls back; any remaining wrapper forwards to the original function with the fast path disabled. Relays remain valid for process lifetime.
