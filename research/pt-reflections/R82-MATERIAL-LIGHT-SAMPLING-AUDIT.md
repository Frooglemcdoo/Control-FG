# R82 material-aware light selection

## Capture evidence

Input: PT-Primary-M12R81-Results-20261016-102526-696.zip and fresh
20261016-1523-58.6476384.mp4. R81 sampled guide submissions: 20 accepted,
no sampled failures; 17 diffuse-guide records include early PT-off and subsequent
bound=1/recorded_pending_submission. No reconstruction setup failures or consumer
rejections. Later light totals: 9 points, 28 spots, zero sun/environment.
Native RR evaluation succeeds with no repeated reset during active PT. Sampled
path faults still include unsupported BRDF and primary shading-normal rejection.
These data rule out the previously observed submission failure in this capture,
not every possible per-pixel fallback. The video is not a synchronized GPU capture.

## Concrete source finding

PTPathLight selected four uniform candidates with target luminance(Li)*cosine.
Final PTEvaluateDirect multiplies Li by a diffuse+GGX BRDF, whose narrow specular
response depends on view, material and roughness. The old target is unbiased with
full support, but can select low-contribution candidates over a glossy highlight.
This is a variance weakness, not proof that it causes all observed flicker.

NVIDIA's application bridge explicitly permits approximate BRDF targets but warns
that insensitive weights are noisy:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RtxdiApplicationBridge.md
(section RAB_GetLightSampleTargetPdfForSurface).

## Change and bounds

Pass outgoing world direction to the light adapter. Construct the same tangent
frame as the integrator and evaluate each candidate with PTLightTarget, shared
HLSL/C++ math using PTEvaluateDirect with unit PDF and visibility=1. Reservoir
normalization remains count*selectedTarget/sum(target/proposalPdf). Shadow
visibility is tested once after selection. No candidate count, bounce, ray count,
RR history, source ownership or native/PT gate changes.

The shader fixture adapters accept the new direction argument, retaining their
fixed lighting. Both native and replacement shader builds must compile. New
Monte Carlo regression compares old/new estimators to an exact 16-light glossy
sum using identical proposals and selection variates. 500,000 trials: expected
27.316792; old/new means 27.458246/27.329854; variance 7992.142647/2733.061748.
Existing zero/invalid/sparse/occluded regressions remain. This evidence does not
predict game FPS or establish visual completion. Extra per-candidate BRDF work
can increase arithmetic cost; unchanged ray counts do not mean free performance.

Environment scale remains zero and area/emissive NEE is missing. Do not substitute
an arbitrary scale or claim this sampling correction restores missing emitters.
