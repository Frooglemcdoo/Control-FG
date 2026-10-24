# R132: in-game capture with the validated R131 input shader

R130 failed during input pipeline creation on the RTX 5090 with driver
32.0.16.1742. R131 removed obsolete payload instructions and linked only the
input export. The user's R131 log passed all five hardware stages, including
input creation, GPU table translation/readback and descriptor forwarding.
This confirms standalone startup, not game-scene traversal.

R132 enables GPU capture in this experimental build with the exact R131 shader:
SHA-256 `472a3acd48496acbfda713d421de95a6e9e8bc188946389bf476239cf1e31684`.
The diagnostic build flag is explicit; ordinary builds remain isolated.
The native ray calculations, 48-byte input records and 80-byte output records
are unchanged. Centered shadows, GI filtering, character guides, glass and the
60% clamp are unchanged. This is a capture test, not a lighting correction.

## Test

Close Control and copy the package beside Control_DX12.exe, preserving the
existing NVIDIA runtimes and ControlFGStreamline files. Load the affected room
with RT Contact Shadows on, samples 1, FG and glass enhancements off. Face the
problem light/table/floor. Open F10 and select BOTH once, then close the overlay.
Hold the view for a few seconds and let the game run for 70 seconds to export.
Send the single `Lighting-R132-*.log` from
`%LOCALAPPDATA%\Control-FG\Diagnostics`. Logging starts automatically;
SAVE LOG is optional. If the game crashes, send that log anyway.

At most one 64-by-64 GPU hit capture runs per process. Subsequent mode changes
do not reallocate or overwrite it. No GPU wait is added to the render thread;
readback requires completion of the actual submitting queue's fence. Existing
replay/reset rejection and retained resources remain in effect.

## Expected evidence

- `rt_hit_gpu_pipelines ready=1`: input and translation pipelines created.
- `rt_hit_gpu_requested`: BOTH requested the one-shot capture.
- `rt_hit_gpu_recording`: commands recorded; not proof of GPU completion.
- `rt_hit_gpu_readback`: completion and record validity checks.
- `rt_hit_gpu_export_complete`: result export finished.
- `rt_hit_gpu_pending` / `rt_hit_gpu_failed`: the specific unmet condition.

CI checks both disabled and enabled build policies, queue/generation checks,
log export retry, GPU table translation and standalone pipeline creation.
Only the live game log can establish that native bindings and traversal work.
