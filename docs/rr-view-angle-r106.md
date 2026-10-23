# R106: world specular-guide view-angle comparison

## Test

Install with Control closed. Keep RR on, native RT Contact Shadows on, FG and glass enhancements off. In F10 compare WORLD ALBEDO with VIEW ANGLE from the same position. Let each settle for ten seconds, then move. No restart between modes. Save one Lighting-R106 log after testing and send paired images or a video. Test both a central painting and an off-center wall.

Both modes retain the fixed 60% clamp, GI filtering, textured Standard-material diffuse albedo, and centered filtered spotlight shadows. VIEW ANGLE replaces the rejected SPEC FILTER button; it does not re-enable that filter. No new shadow assets or NVIDIA runtimes.

## Isolated change

The baseline EnvBRDF lookup uses abs(view-normal.z). VIEW ANGLE uses abs(dot(view-normal, normalized pixel view ray)) for the four exact-match Standard world replay shaders. Native F0 decoding, LUT texture and smoothness axis, BRDF-mode exclusion, diffuse RGB, normals, roughness, skin, hair and responsivity stay unchanged. The private replay alpha is now -4 solely to identify those audited world pixels; valid RGB, not alpha, remains the coverage test. Native world shaders are never changed.

The owned guide constant block grows from 16 to 32 bytes and carries the current camera's two perspective scales. Both forward/inverse matrices must validate and match the captured symmetric positive-Z perspective shape. Unsupported matrices, missing world replay or stale camera frames retain baseline guides. Each fence-protected slot records the actual guide variant; RR history resets when the consumed variant changes, including fallback. There is no new render-thread readback, allocation, shader compilation, GPU wait or file write.

## Evidence and limits

This is a diagnostic, not a confirmed fix for every oversharpened surface. It uses unjittered pixel-center rays from the captured camera. Native NGX jitter is deliberately not applied with an unverified sign; exact subpixel correspondence remains to be audited. This tests the larger off-axis viewing-direction approximation, not a complete temporal/jitter correction. The existing LUT's absolute-cosine convention and native smoothness axis remain unchanged. The central painting had a much smaller predicted angle error than the off-axis wall, so no difference there would not invalidate the implementation.

CPU counters distinguish view_angle_recorded, view_angle_fallback, view_angle_consumed and view_angle_baseline. They establish that the intended same-frame guide path ran; they do not measure the number of affected visible pixels or prove visual quality. All four replay shaders retain exact native RGB instructions. WARP validation executes the production shader against a known LUT and independent CPU ray calculation, checks missing/invalid replay fallback, and checks skin/hair and all non-specular outputs remain unchanged.

All fourteen spotlight assets and three glass assets must remain byte-identical to R97; the compiled clamp remains 60%. In-game image quality and hardware stability still require the user's comparison.

Research background: [R105 investigation](https://github.com/Frooglemcdoo/Control-FG/blob/b27080e50a83f42a7122955c1eeb0c87d708c99a/docs/rr-material-sharpening-r105.md). NVIDIA reference: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md . No SDK upgrade is part of this test.
