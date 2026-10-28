# R204: Native Blend ray-origin comparison

R203's completed frame 304119 contains 4096 supported paired samples. All 547
samples with native-map visibility more than .25 above filtered RT belong to
capture-local light 21. Of the 391 with a lit native center and blocked raw RT,
287 clear along the matrix-derived projection path and 104 remain blocked.
Projection also hits 13 of the 53 samples where native center and raw RT are
both lit. This supports a path discrepancy but does not justify a default fix.
The selected light changes near the right-hand boundary; the exact seam and
reflection appearance are not resolved by these counts.

## Comparison

Native Blend still starts with emitter rays. Keep RR ON, SPEC Native, hit
distance OFF, accepted RT effects, and MAP GUARD OFF for this comparison.
At the same wall view, use F10 -> RAYS: EMITTER / RAYS: PROJECTION. Close the
panel and allow settling after each change. Check the shadow angle, missing
light, right-hand cutoff, Jesse's close shadows and any newly dark areas.
Send matched screenshots. TRACE SHADOW works in either setting and records the
selected target; a trace from each setting is useful but no inside-room trip or
F9 reflection suite is required. Switch back to EMITTER if the result is worse.

MAP GUARD remains independent. Switching it preserves the ray target; switching
rays preserves the guard setting. Both stay inside Native Blend with RR and the
same correction/filter/sample policy. Switching is refused if the optional
projection pipeline is unavailable. Capture preserves the selected mode and
cancels on a mode/settings change.

## Implementation and limits

The comparison reuses the verified native cone-apex ray shader introduced in
R162, now selectable inside Native Blend rather than its old RR-off mode.
The cone apex agrees with the independently solved projection apex to within
1.334e-6 engine units across all 4096 R203 samples. This is evidence for these
captured lights, not a universal assertion about every authored light. Do not
infer exact parity with shadow-map receiver bias, raster casters or PCF.

The projection shader is independently clipped per path. Its four identical
centered traces are reduced to one using the same proven reuse transformation
as the accepted emitter path. Point rays, handlers, visibility aggregation and
all composition/filter/reflection shaders are retained. No room coordinates,
light IDs or mesh exceptions are used. Emitter remains the startup default.

Validation covers mode readiness, RR policy, map-guard independence, capture
eligibility, stale observations, PSO routing and ray-table bounds. The exact
projection transform and one-trace reuse are validated during the Windows
build. The prior R162 visual failure remains relevant; this is a controlled
comparison informed by new paired-ray evidence, not a confirmed lighting fix.
