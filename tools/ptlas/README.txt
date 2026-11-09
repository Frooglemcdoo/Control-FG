CONTROL PTLAS CAPTURE REPLAY R394

Standalone validation using the already collected R393 instance descriptors.
This does not install into Control or change your game settings.

Close Control, extract this folder anywhere, and run Run-PTLAS-Validation.cmd.
Return PTLAS-Replay.txt, whether it passes or fails. No gameplay/capture needed.

The package includes your existing capture.bin. The CPU replay checks all source
bytes and slot removals. The GPU test compares native TLAS with PTLAS using the
captured transforms, masks, IDs and slot order, including native NaN transforms.
All captured BLAS addresses are replaced by an owned local test triangle. Game
geometry and shader-table execution are NOT reproduced. It probes 128 targeted
rays per snapshot; it is a compatibility test, not an FPS benchmark or proof of
complete traversal coverage. It never accesses captured process addresses.

NaN acceptance is enabled only in this standalone validator; the default planner
still rejects nonfinite transforms. Native preparation retirement, live resource
lifetimes and game shader consumers remain unproven. No PTLAS game switch is added.
