# R114: Tighter spotlight endpoint test

R113's END GUARD restored the missing cafeteria light, but the user reported unacceptable shadow artifacts. The supplied screenshot shows broad shadow shapes across the wall and lockers. Its log records endpoint readiness and the endpoint ray record at offset 128; all 8,397 RR evaluations succeeded. The screenshot marked RAW is a different viewpoint, so it is not a matched image comparison.

R114 reduces the exclusion to `min(0.1 * positiveEmitterRadius, 0.01 * rayLength)`, one tenth of R113's exclusion. The button is now **TIGHT END**. This remains a diagnostic: it may retain the artifacts, or restore the unwanted darkening. It does not identify a fixture mesh or establish that the light center is misplaced. Do not treat it as a production fix until both brightness and shadow shape pass.

## Test

Close Control to install. Keep RR on, native RT contact shadows on, game Ray Tracing Samples Count at 1, saved GI samples unchanged, and FG/glass enhancements off. No restart is needed between modes.

1. At the cafeteria table/floor viewpoint, compare **BOTH**, **TIGHT END**, and **GI ONLY**. Return to gameplay and allow ten seconds per mode.
2. Repeat those same modes at the wall/lockers viewpoint from the artifact screenshot. Keep the camera fixed during each comparison.
3. Check Jesse, chair legs, and objects near ceiling fixtures for missing shadows, abrupt edges or changes while moving.
4. Save one **Lighting-R114** log and send the two viewpoint comparisons. Report separately whether the light returns and whether the shadow artifacts remain.

All room comparisons retain SPEC 4, saved GI samples, GI filtering and material guides. RAW and LIGHT OPEN remain diagnostics. LIGHT OPEN intentionally removes the selected spotlight's shadow. SPEC 8/16 are separate quality references, not part of this endpoint comparison.

## Scope and evidence

Only four spotlight endpoint equations change relative to the hash-locked R97 centered shader. The receiver origin, center direction, four samples, closest-hit search, native hit handlers, blocker-distance aggregation, penumbra inputs and filtering are unchanged. Existing fourteen spotlight sidecars, three glass assets, skin/hair guides and RR's 60% clamp are unchanged. The optional shader remains embedded and uses the existing state-creation fallback. No new per-frame allocations, GPU readbacks, waits or rays are added.

The emitter radius at native light offset 72 is a size input, not a geometric error bound. Using it to shorten rays is deliberately an experiment. Standard area-light visibility approaches offset the endpoint using surface/error information to avoid self-intersection while retaining nearby blockers; we do not yet have an identified emitting surface here. Reference: https://www.pbr-book.org/4ed/Shapes/Managing_Rounding_Error . A smaller cutoff is a controlled next measurement, not a substitute for that geometric information.

Validation includes signed DXIL validation, exact reversal of all four transformations back to the original complete disassembly, zero/negative/NaN/infinite radius cases, segment bounds, hit/miss consistency and a near-emitter blocker that R113 discarded but R114 retains. Shader generation and runtime logging share the manifest's constants. Existing CI checks exercise mode readiness, PSO routing and record bounds. These checks cannot prove image quality on hardware DXR.

If TIGHT END restores both light and valid shadows, test near fixtures and in other rooms before promoting it. If brightness returns but artifacts remain, capture blocker/endpoint evidence next rather than increasing the exclusion. If it becomes dark again, that brackets the problematic interval between the R113 and R114 cutoffs; it still does not identify which mesh caused the hit.
