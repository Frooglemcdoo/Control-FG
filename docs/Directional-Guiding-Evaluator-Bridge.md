# Guiding evaluator bridge — 2026-11-11

Research only. No change to the R423 runtime, no new game package, and no
demonstrated reduction in Jesse's motion noise or frame time.

## Located boundary

`src/shaders/gi_owned_lighting.hlsl` saves primary throughput immediately after
`GIInitializeReceiver`, calls `GIEvaluateSecondary` for each native ray slice,
then accumulates `throughput * receiver.r15.xyz`. Depth fade and division by
the original ray count happen after the loop. Both surface shading and volume
lighting on misses pass through that same accumulation point.

The native throughput is already compensated for native sampling. Changing the
angular proposal requires the **native angular PDF / actual guided angular
PDF** ratio exactly once, not division by the guided PDF alone. This was already
part of the earlier accounting contract; it is not evidence of a newly found
bug in the shipped code. The independent native finite-range draw must remain
unchanged for an angular-only correction to be sufficient.

`src/gi_reuse/guiding_contribution.h` implements this accounting operation and
returns corrected RGB plus its luminance for learning. Legitimate zero terms
remain valid and count in the denominator. Invalid or overflowing terms are
rejected in the research fixture, never silently treated as unguided rays.
Production must establish readiness before emitting guided rays: falling back
to weight 1 after emission is not a valid recovery strategy.

## Saved-data validation

The input is the existing `research/gi-source-r267/candidate-gpu-fixture.json`:
nine small fixtures containing 46 rays, including trace-derived surface/miss
lighting and two synthetic accounting edge cases. These are NOT the R423 jaw
and hair captures, and contain no captured guided PDFs. Test PDF ratios are
synthetic (unit, varying 0.25/1/2, and a legitimate zero).

[Windows run 43022868194](https://github.com/Frooglemcdoo/Control-FG/actions/runs/43022868194)
at `12d65cc8f5896fed4fd3d161443cb34df5844e12` passed:

- Nine fixtures × three correction patterns.
- Unit correction recovers saved native RGB; maximum absolute error 2.98023e-08.
- Independently computed surface and volume-miss sums, original denominators,
  and learning values before display depth fade.
- Ten invalid packets: stale frame/generation, wrong ray order, invalid PDFs,
  nonfinite lighting, invalid material sentinel/count, and overflow.
- Dispatch bounds and untouched output guard.

The first compiler run rejected a 3088-byte structured-buffer element. The
fixture now reads scalar words rather than exceeding the 2048-byte element
limit. This is a test storage correction, not a production memory layout.

The two-frame sampling fixture now calls the same contribution helper instead
of maintaining a separate multiplication. Its illumination remains synthetic;
it does not execute the full Control material/light evaluator.

The combined version passed [Windows run 43023181082](https://github.com/Frooglemcdoo/Control-FG/actions/runs/43023181082)
at `5279cae24c1ea435c63052ef41a1d3e85e3367d5`: 640 two-frame proposals,
401 changed next-frame PDFs, and all captured accounting checks above.

## Distance and provenance constraints

`GIEvaluateSecondaryCore` separately accumulates native hit information in
`r5.w`; misses use `giCB4[0].x`. Do not multiply that register by the RGB
correction without a separate distance-estimator policy.

The inspected RR guide setter explicitly supplies optional **specular** hit
distance (`DLSSD.SpecularHitDistance`), not this GI hit-info texture. The NRD
input adapter does read native `g_tHitInfo`. Therefore the GI output is not
established as a directly supplied RR diffuse-distance guide. Native denoiser
and optional NRD compatibility still require a distance audit before shipping.

The producer must export the sampled world-direction cell and its probability
ratio from the frozen proposal. Reconstructing them later from RR-smoothed
normals, changed guide tables, or transient raw material IDs is not acceptable.
The frame/generation fields in this fixture check same-frame provenance only;
they do not solve persistent receiver tracking across motion.

## Remaining gate

Actual per-ray binding, scalable learning, surface correspondence, changing
illumination rejection, resource retirement and RTX cost remain unimplemented.
The previous motion-learning failure remains relevant: correct accounting alone
does not prevent a stale guide from increasing variance. No live enablement or
user capture request is justified by these accounting results alone.
