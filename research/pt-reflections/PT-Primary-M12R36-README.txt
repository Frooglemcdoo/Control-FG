Control primary PT - M12R36

Close Control and replace all package files. Compare PT off/on/off in the same
scene, roughly 20 seconds each, then run Collect-PT-Primary-M12R36.cmd.
F10/F11 controls are unchanged. Keep resolution and other settings fixed.

R35 did not resolve the reported FPS regression. CPU preparation remained
roughly 6-9 ms in the logged samples while displayed PT GPU dispatch was about
2 ms. Those measurements are not a complete frame profile.

R36 changes scene metadata cleanup: previously EVERY observed Execute, Signal
and Wait appended an extra private GPU fence signal, including PT-off mode.
Now each queue samples its first nonempty graph tail, then at most once per 64
operations when its graph stamp advances. Cleanup uses only the last actually
polled sample. Skipped operations never imply completion. Independent geometry
retirement, PT admission, native queue waits and scene-use fences are unchanged.
The queue-order regression exercises 20,000 writes with 313 cleanup signals,
bounded metadata, and rejection of falsely inferred future completion.

New cumulative PT_PRIMARY_QUEUE_CPU records separate PT off/on and Execute/
fence operations, reporting queue-mutex wait, observer/callback CPU duration,
and native driver call duration. Maxima are per operation, not per frame.
Observer totals INCLUDE preparation/validation callbacks and must not be added
to their separate timings. This does not measure all hooks or GPU idle time.
PT_PRIMARY_PROGRESS records actual/skipped cleanup signals. Both records are
included in the collector summary. These measurements will help identify the
remaining cost instead of assuming the PT shader is the whole bottleneck.

FPS improvement still requires an in-game check. PT-off retains geometry
capture to avoid stale geometry when re-enabled. Lighting replacement is still
unfinished: native primary lighting/shadows remain and PT indirect is additive.
This build does not claim corrected lighting or new soft shadows.
