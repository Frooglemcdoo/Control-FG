# R139 — direct-light surface-normal correction

In the R138 capture, Bent Normal Off removed the broad dark patch around Jesse
in all three images while narrower shadows remained. Local Maps Off, Contact
Off, Screen AO Off and RR Off retained the patch. Both modified lighting passes
were recorded; the suite completed all 60 images without failures. This supports
correcting the direct-light bent-normal contribution. It does not prove the
underlying bent-normal texture is invalid or establish behavior in every room.

## New default

**Default (Fixed)** now combines the working authored-clip ray path with the
exact surface-normal shader variants tested in R138. Direct spot/point shading
uses the decoded surface normal instead of the occlusion-adjusted bent normal.
This applies across the supported lighting passes, without room, light-ID or
material-ID exceptions. No new shader math is introduced in this build.

RT visibility, native nonselected shadow maps, scalar AO, GI filtering, light
cookies and material calculations remain active. The indirect-light bent-normal
paths are not changed. Ray counts, ray libraries, spotlight filters, skin/RR
guides, reflection clamp and glass sidecars remain byte-identical.

The correction activates automatically once both normal PSOs, the authored-clip
rays and sample hooks are ready, unless the user has already selected a mode.
Missing prerequisites leave the mode unavailable rather than claiming it applied.

## Compare and capture

Install with Control closed. Keep RT Contact Shadows on, RT samples 1 and the
same RR/GI settings. Default (Fixed) is the corrected appearance; **Clip Only**
is the R138 default for comparison. Native and Both retain their prior meanings.
Check the original location plus other rooms, including moving around Jesse
and lights. Verify the broad patch is gone without losing legitimate shadows.

F9 now runs 21 comparisons (up to 63 PNGs), including the new default with RR on
and temporarily bypassed. Keep the camera still until completion (about 100
seconds), then zip the displayed `Shadow-R139-...` folder. The prior mode and
saved RR preference are restored. F9 again or focus loss cancels.

## Validation

R138's shader identity/operand checks, WARP shader creation and live route tests
remain in CI. R139 additionally checks default prerequisite gating, the corrected
RR-off route and capture readiness for both lighting passes. Existing correction
binaries and all shipped lighting sidecars are verified unchanged. Broader
in-game visual validation is still needed; this is an experimental test build.
