# R117: Native hit-position bands

R116 confirms the affected table/floor regions are predominantly in the R113-only endpoint interval. RR bypass worked: the success counter remained 1,743 during all signal views and resumed after BOTH. This establishes a near-light hit interval, not which mesh is hit. Native ShadowHitData contains a single float distance; no instance or primitive ID is returned to ray generation. R117 reconstructs the hit position without changing that payload, hit groups or alpha testing.

## Test

Close Control to install. Keep FG/glass enhancements off, native RT Contact Shadows on and game RT Samples Count at 1. RR bypass is automatic for signal views and preserves the saved setting. No restart between modes.

At the cafeteria table/floor viewpoint, take **HIT BANDS**, **HIT X**, **HIT Y**, **HIT Z**, keeping the camera still. Repeat at the wall/lockers viewpoint. Return to gameplay and allow ten seconds per mode. Finish with **BOTH** and save one **Lighting-R117** log.

The color legend differs between the original HIT BANDS and the new coordinate views:

| Color | HIT X / HIT Y / HIT Z | HIT BANDS |
|---|---|---|
| Blue | Hit coordinate is less than light coordinate by more than 0.1 emitter radius | Earlier blocker |
| Orange/yellow | Coordinate is within +/-0.1 emitter radius of the light's plane on that axis | R113-only endpoint interval |
| Red | Hit coordinate is greater than light coordinate by more than 0.1 emitter radius | R114 tight interval |
| Green | No hit on the full centered ray | No hit |
| Magenta | No selected spotlight | No selected spotlight |

Coordinates use the native world axes; X/Y/Z are not camera left/up/forward. These are coarse signed coordinate bands, not exact coordinate readouts or mesh identity. Read them primarily over the orange regions in HIT BANDS. Material shading and tone mapping still affect displayed intensity. LIGHT IDS remains available to distinguish selected lights. BOTH restores normal centered shadows and RR eligibility.

## Implementation

For each axis, the first centered ray's closest native hit yields `hitMinusLight = (hitT - fullSegmentLength) * rayDirectionAxis`. This equals `rayOrigin + hitT * rayDirection - lightCenter`, avoiding subtraction of large world coordinates. Use the full segment length, not the epsilon-shortened TMax. Classify against +/-0.1 times the nonnegative native emitter radius; equality is in the center band. Miss classification takes precedence. Negative or NaN radius becomes zero; an infinite radius gives an uninformative center band. No world unit scale is assumed.

Three optional DXIL libraries append records at offsets 192, 256, 320; the immutable upload record buffer grows from 192 to 384 bytes. The old centered/R94/blocker records retain offsets 0/64/128. If position-library state creation fails, retry with the R116 libraries; if the blocker library fails, retry the original augmented state. Buttons require the position records and existing color composition to be ready. Dispatch checks actual buffer bounds. No additional rays, per-frame allocation, GPU readback, waits or synchronous logging are added.

Only spotlight visibility output changes in these diagnostic shaders. Full ray endpoints, receiver bias, hit/miss/alpha-test handlers, point work and mean blocker distance are preserved. The existing categorical color shader is reused. All fourteen canonical shadow sidecars, three glass sidecars, skin/hair guides and RR clamp remain unchanged. Specular quality modes remain implemented but their buttons are replaced by coordinate diagnostics for this test.

Validation covers all three signed DXIL assets, complete source reversal to hash-locked R97, independent Cartesian reconstruction across translated scenes, sign and equality boundaries, misses, mode/readiness gates, RR preference restoration, all six record bounds, WARP composition routing and the 132-file baseline guard. These do not establish hardware image correctness. This is an intermediate position measurement, not a geometry-ID capture or a lighting fix. A broad constant coordinate-side pattern across the orange regions supports a surface near a light plane; identifying and excluding an actual fixture requires further geometry evidence.
