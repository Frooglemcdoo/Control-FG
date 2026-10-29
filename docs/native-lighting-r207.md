# R207: spatial GI filtering with per-frame history clear

R206 bypass removed the reported moving-character ghosting but restored RR
speckles. R207 restores the R205 GI filter policy and clears only the GI history
before each Native Blend GI filter under full RR. The native temporal dispatch
still runs for its initialization/output work, followed by the unchanged native
spatial passes. This tests removal of prior-frame GI accumulation; it is not a
claim that spatial filtering alone is sufficient or that ghosting is fixed.

## Evidence and implementation

The hash-verified renderer's diffuse-filter.asm shows:
- GI history slot renderer+0x90eac8 supplied at the existing +0x12cd96 hook.
- +0x15130 checks for absent history; allocation clears it to float4(0) through
  getNativeTexture (+0x151d6) and clearUAV (+0x151e3).
- temporal_feedback binds at +0x15691 and dispatches at +0x156d3.
- filter_color spatial passes follow, with the original pass count and output copy.

R207 reuses that zero-initialization operation on an existing history texture
before the filter runs. The native texture pointer and clear functions are
resolved from the verified d3d module and checked against renderer IAT slots
+0x5dfd30 and +0x5dfd40. Only the exact GI history slot is accepted. An absent
history is left for the native allocation/clear path. No ownership changes,
per-frame history allocation, GPU waits, RR reset-every-frame, specular-history
clear, or new shader replacement is introduced. The extra clear has a GPU cost
that remains to be measured. Native pool synchronization remains engine-owned.

The request is latched with the GI frame policy. It requires selected RR,
Native Blend, a valid color, positive spatial pass count and the lighting stage.
Clear failure is logged and fails the RR frame; the native filter still runs.
Other lighting modes and RR-off behavior retain their existing filter paths.
The exact baseline exception records the narrow GI integration edits.

Projection rays, MAP GUARD OFF, pre-R184 reflection view, world-albedo guides,
GI sample count and spotlight filtering match the accepted R205 baseline.

## Test

Install R207 with the game closed. Use Native Blend, RR ON, SPEC Native,
projection rays, MAP GUARD OFF, hit distance OFF and frame generation OFF.
Keep the game's indirect diffuse lighting enabled.

Repeat both the carpet movement that showed ghosting and the projector-room
floor that showed speckles in R206. Wait for loading to settle. Record a short
movement clip and a few seconds after stopping; take a floor screenshot and
F10 SAVE LOG. No F9 suite is needed. Compare with R205 and R206 using the same
settings. Report trailing, stationary noise, brightness/detail changes and FPS.

Logs should show gi_spatial_only build=R207 applied=1 with increasing cleared
counts, zero failures, and lighting_policy gi_filter=1 under active RR. These
are CPU recording evidence, not proof of GPU image quality. Unchanged component
logs can retain older build labels.

Local tests cover exact history identity, missing-history initialization,
clear failures, native filter forwarding/order, mode scope, LastError and
baseline boundaries. Windows CI compiles the native adapter and runs existing
WARP/resource, mode-switching and capture checks. In-game validation is required.
