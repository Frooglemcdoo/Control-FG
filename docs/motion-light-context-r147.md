# R147 — isolate baked-GI fill while preserving corrected shadows

R146 completed all six user motion samples with zero failures and 24,576 valid
records. Closest matched receivers did not show broad selected-spot visibility
loss. This does not establish exposure as the cause or exclude local failures.
See motion-light-context-r146-review.md for the measurements and limits.

R147 adds two live comparisons: BAKED GI OFF and GI + RR OFF. Both retain the
accepted surface-normal producer/composition, authored ray clipping, current-frame
spot visibility, centered spatial softness, native point history and GI filtering.
The second also bypasses RR without modifying the saved RR preference. Only the
already hash-verified native baked-GI pixel-shader contribution is removed. This
is not an all-indirect-lighting switch. Exposure adaptation remains enabled and
can respond to the changed image; improvement alone is not proof of a bad probe.
Default rendering and all shipped shader sidecars remain unchanged.

A separate observation bit confirms the baked-GI override was actually bound.
Producer/composition evidence alone cannot mark the comparison applied. Missing
variants disable availability; missing bindings remain waiting. Returning to
Default restores the native baked contribution. Old worker observations cannot
confirm a new selection. These are CPU binding observations, not GPU completion.

## Test

Close Control and install beside Control_DX12.exe; preserve NVIDIA runtimes.
Use RT Contact Shadows on, samples 1, FG/glass enhancements off.
Record one video repeating the affected doorway movement in these four modes:

1. DEFAULT (FIXED).
2. BAKED GI OFF.
3. RR OFF.
4. GI + RR OFF.

Select using F10, close the panel and allow the image to settle before walking.
No restart is needed. Compare whether the same shadow remains visible through
the movement, not absolute brightness between modes. Return to Default and Save
Log; send the video and log. If a mode stays waiting, send the log rather than
treating it as an unchanged result.

Raw Signal and Raw Shadows remain in F9. F9 now includes 30 stationary comparisons
(up to 90 PNGs). Shift+F9 retains the verified R146 six-sample Default motion
capture; it does not cycle the new modes. Package-Lighting-Capture.cmd still
splits captures into ZIPs below 200 MB, retaining originals.

## Validation

Local policy tests cover required variants, independent binding evidence, stale
observations, RR bypass and preservation of accepted shadow policies. Windows/WARP
also checks corrected shader routing, executed GI override/restoration through
both SetPipelineState and Reset, missing-variant fallback and existing capture
scheduling. In-game confirmation of this comparison remains pending.
