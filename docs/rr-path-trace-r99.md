# R99: identify the active RR lighting paths

R98 did not improve the projector-room speckles. Its log confirms the selected shadow modes but contains no broad diffuse hook execution records. That does not validate a successful native diffuse restoration. R99 adds independent entry and completion counters to find out which code actually executes.

## Test

Install with Control closed. Keep native RT Contact Shadows enabled and FG/glass enhancements off. Load the projector room, select CENTER FILTERED and leave that shadow mode selected.

Keep RR ON for 15 seconds while looking at the noisy area. Switch RR OFF for 15 seconds, then ON for another 15 seconds. No restart is required. SAVE LOG once and send the resulting Lighting-R99 log. We already have the visual comparison; screenshots are optional unless the appearance changes unexpectedly.

R99 is an observation build: no improvement in noise is expected. It retains the R98 controls and exact R97 shadow assets.

## Evidence in the single log

`rr_path_snapshot` and `rr_path_count` records are written every five seconds, before shadow-mode changes and when saving. Counts are lifetime totals, never reset by the UI. Compare consecutive snapshots to find active paths. Each count is an independent atomic observation; a snapshot is not a transaction or a single GPU frame. `rt_effects` and `rr_selected` are the latest reset-hook observations, and `last_result` is the last returned native evaluation result, which may be SR or RR.

- `hooks_installed`: successful installation of the full frame-hook set.
- `broad_a/b`: entries to the two broad diffuse hooks, even before the original-function check. `broad_native/bypass`: completed original call versus skipped call.
- `reflection_entry/native/clamp/raw`: entry to the reflection filter and completed native filter, clamp dispatch call or raw substitution path. CPU return does not prove GPU completion.
- `gi_option/bypass_selected`: calls and true results from the separate GI bypass getter. It returns exactly the previous shared getter result. `gi_release/validated`: release-hook entry and successful existing same-frame bypass contract.
- `evaluation_entry`: native evaluation gateway entries. `rr_attempt/success/failure`: RR submissions and returned API status. `sr_return`: returned SR-branch calls. `evaluation_rejected`: contract rejection before invoking evaluation.

Counters use fixed lock-free atomics. They do not access textures, allocate resources, insert GPU work, wait, or write files from the render thread. The existing diagnostic UI thread snapshots the counts and drains the bounded log queue.

No RR guide, skin, clamp, GI filtering, shadow filtering, model, shader asset or FG behavior is intentionally changed. WARP/compile checks cannot establish game image quality or hardware stability.
