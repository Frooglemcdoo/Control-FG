# R166: performance capture

Click **PERF CAPTURE** in F10 during gameplay with the desired lighting mode and RR settings already selected. The panel hides and returns focus to the game. A five-second foreground settling period precedes 20 seconds of sparse GPU sampling (every 16 presents). Walk through the affected room and doorways. Keep graphics settings unchanged.

The capture does not change modes, RR, ray samples, shadow softness, resolution, GI filtering, or history. All rendering shader bytes match R165. It adds no room culling.

After timing, one optional 4096-pixel light-context capture and a display-referred PNG are collected. This additional diagnostic work is excluded from timing. A bounded wait for completion occurs only through UI polling: no GPU waits or file I/O in rendering callbacks. The overlay offers cancellation when reopened.

The automatically created ZIP includes:
- `gpu-timings.csv`: fence-confirmed per-dispatch GPU durations, native present, mode, RR selection, RT flags, dimensions and hit-table bytes.
- `manifest.txt`: timing scope, errors, exclusions, sample counts and per-dispatch means.
- `lighting.log`: current policy, successful RR evaluations, native sample settings and optional light context.
- `runtime.log`: this capture's main-log tail, including existing RR performance measurements when emitted.
- `scene.png`, installed DLL/shader hashes and GPU/driver identity.

Stage labels separate the shadow-ray dispatch, other ray dispatches, selector, history, two spatial filters and composition. Shadow rays currently include the combined spotlight/point shader; launch dimensions are not actual traced-ray counts. The context snapshot does not identify engine rooms or prove visibility through walls. This package supplies timing and light-selection evidence for the next optimization; it is not a full PIX capture.

Sparse timestamp queries slightly perturb sampled frames. Compare repeated stationary captures or repeat the same route. Mean dispatch times are not whole-frame costs; multiple dispatches/stages may have different sample populations. Settings changes, focus loss, unsupported queue changes, repeated list execution and incomplete GPU work are marked rather than represented as valid results. No camera coordinates or persistent object identities are inferred.

Timestamp/readback memory and command-list references remain retained to protect any native command-list resubmission. Query slots are never reused; resource usage is bounded. If capacity is exhausted, restart the game before capturing again. GPU readback faults disable profiling without changing rendering.

The output ZIP is validated below 200 MB; originals remain if packaging fails. If that limit is exceeded use the included Package-Lighting-Capture.cmd splitter. Package-Lighting-Performance.ps1 is included beside dxgi.dll and requires only Windows PowerShell/.NET already present on Windows 10.
