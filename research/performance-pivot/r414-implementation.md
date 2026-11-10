# R414: bounded rendering measurements

The current conversion allocates six full-resolution textures: radiance/hit
RGBA16F, normal/roughness RGBA16F, depth R32F, motion RGBA16F, remodulation
RGBA16F, and filtered radiance RGBA16F. That is 44 bytes/pixel of texture payload
before resource alignment, NRD pools and receiver-export resources: about
154.7 MiB at 2560x1440. This describes allocation size, not measured memory
traffic or a demonstrated bottleneck. No format or shader is changed in R414.

The pipeline separately measures input transitions/conversion, NRD dispatches
with their preceding barriers, and output conversion/state restoration. The
NRD aggregate includes the dispatch scopes. The outer RELAX span also includes
surrounding command recording and native binding restoration. GPU timestamp
commands themselves may affect results; nested durations must not be added.

Callbacks are absent outside sampled automatic-comparison frames. Existing
fence retirement, exact command-list submission and non-recycled query indices
are retained. The query pool remains bounded at 16,384 spans. No GPU waits,
stack walks, synchronous per-frame file writes or texture readbacks are added.
A no-query/no-CPU-span-timer full phase followed by a profiled full phase tests
this measurement cost while retaining cadence and coverage counters. It cannot
prove that all diagnostic or loaded-mod overhead is absent.

The obsolete four-sample condition becomes a native-sample confirmation. Two
loaded-SR phases and a return to RR provide a net integration comparison;
RR/SR successes and failures are checked separately. Temporary RR mode changes
bypass bundle-default setters so unrelated user settings are not reset. The
saved mode is restored on completion or cancellation. The entire run is marked
incomplete for missing coverage/timing, skipped queries, dropped rows, profiler
faults, insufficient cadence rows or more than 5% repeat drift.

R413 root CPU configuration is held fixed, including owned preparation and
recording/binding replacements. No new CPU ownership claim or A/B package is
introduced. Build flags are in the report. This is NOT an untouched-engine
baseline, an exact snapshot of every accepted visual feature, a total GPU frame
timer, or a traversal capture. Those evidence gaps remain explicit.

Validation additions exercise nested real D3D12 timestamp retirement and metadata
on WARP, and instrument the actual NRD transaction while preserving its existing
output, state restoration, frame restart and buffer reuse assertions. Local
portable checks cover native-only sample policy, phase modes, drift validity,
cadence recording and generation-bounded settings evidence. Windows/GPU results
must come from CI; local portable success is not a live-game result.
