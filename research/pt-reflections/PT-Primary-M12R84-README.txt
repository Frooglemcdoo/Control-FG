M12R84 - identify draw-level PT/native fallback

Diagnostic build: rendering, ray counts, RR guides, coverage and settings policies
are unchanged from R83. No visual improvement is claimed for this build.

R83 recorded surface_bindings reason_count=600, while 21 logged guide submissions
and 18 RR evaluations succeeded; no consumer rejection or late-expired producer
was recorded. A successful PT producer/RR evaluation does not prove every native
surface draw used the PT companion shader. A rejected draw keeps native lighting.
The 600 count is skipped draws accumulated over the run, NOT 600 failed frames.

PT_PRIMARY_SURFACE_BINDING_FAILURE now names the failure and root parameter:
missing argument, root layout, CBV alignment, missing heap or table bounds.
It includes required and actual descriptor counts for range failures. Details
are throttled per parameter (first eight, then every 600); no per-draw readbacks.
Other previously silent returns now have PT_PRIMARY_SKIP reasons: root mismatch,
engine frame/view upload, missing or ambiguous matching producer, table/output,
and recording availability. These are observations, not relaxed checks.

The collector includes SurfaceBindingFailures. Tests exercise missing arguments,
alignment, table bounds and diagnostic reset after success. Existing GPU/lifetime
checks remain. This build distinguishes CPU draw-selection failures from GPU
per-pixel correspondence fallback and RR reconstruction noise; it does not claim
any of them is the sole visible cause.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Reproduce the flicker normally and run
Collect-PT-Primary-M12R84.cmd. No separate mode matrix is needed.
