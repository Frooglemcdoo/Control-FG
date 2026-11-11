# Directional guiding feasibility after R423

Status: research prototype only. Not a game build or a noise fix. No runtime
include references directional_guide.h. R423 production rendering is unchanged.

## Why this work

R423 captures contain identity reconstruction maps at the RR boundary and bright
GI-fed diffuse samples on Jesse. Captured-input replay leaves the six selected
peaks unchanged because the current radiance gate rejects every neighbor.
R422 already demonstrated that removing that gate is not an acceptable fix.
This motivates reducing source variance while preserving native ray count and
lighting energy, but does not establish incorrect GI arithmetic.

Reviewed existing R275/R278 accounting, R419 sampling audit, R416's rejected
endpoint replacement, R381's rejected history filter and the current Owned
single producer. None should be repeated as a new unproven fix.

## Research distinction

NVIDIA's spatiotemporal blue-noise description explicitly says moving pixels
lose its temporal benefit and retain spatial blue noise. A sequence change is
not enough evidence of a cure for this motion-dependent problem.
https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-2/

PBRT explains both importance sampling and its failure when the proposal poorly
matches illumination. A guide must use the full actual proposal density, and
all contributing terms must retain support. This differs from the retired
selective replacement of old endpoints without corresponding weights.
https://pbr-book.org/4ed/Monte_Carlo_Integration/Improving_Efficiency

## Implemented reference

src/gi_reuse/directional_guide.h is an unwired C++/HLSL-style reference (only C++
compiled so far). A distribution over 8x8 native cosine-UV cells mixes at most
50% learned mass with at least 50% uniform mass. It scales HDR training values
before normalization; invalid/empty training falls back to uniform.

The sampler returns the actual relative density 64*p(cell), and its reciprocal.
The proposal must be frozen before ray generation. The exact same distribution
must be used by evaluation: fresh throughput*secondary_radiance is divided by
this relative density, then the existing depth/ray-count normalization applies.
The native range law stays independent and unchanged. Miss/volume contributions
must also receive the weight. This does not reuse endpoint visibility or return
cached radiance as final lighting. Hit-distance semantics need a separate audit.

No guide learning, texture binding, frame ownership, GPU retirement, raygen
augmentation, evaluator change or runtime fallback integration is implemented.
The linear 64-cell search is a reference, not a claimed efficient GPU sampler.

## Validation and rejection gate

Built with g++ C++17 -O2 -Wall -Wextra -Werror. The one-million-draw test checks
histogram/weighted energy, HDR normalization, full support and invalid inputs.
Analytic integration checks colored surfaces, black visibility, nonzero misses,
light removal and stale proposals. A deliberately unweighted estimator fails.

A toy 64-cell field has one cell at 100 and the others at 0.05. These are NOT
captured directional radiance or predictions for Control.

| Proposal | Two-ray variance |
| --- | ---: |
| Uniform independent | 76.8274 |
| Current Owned two-ray diagonal strata, single-frame model | 75.6079 |
| Correctly matched guide | 1.18193 |
| Guide targeting the old bright cell after it moves | 150.2522 |

The mean remains 1.61171875. Correct weighting preserves mean even when stale,
but does not guarantee lower variance. On constant miss lighting, stale guiding
adds variance where uniform sampling had none. Do not present the matched-guide
number alone as a successful replacement.

learning_trial.py tests previous-update density-corrected learning. With only
two training rays/update, variance ratios against independent uniform are
0.292 static, 1.773 with a hotspot changing every frame, 1.703 every 8 frames,
and 1.498 every 32 frames. A deliberately coherent 128-pixel tile (256 training
rays) improves the static/8/32-frame cases but is still 1.953 with changes every
frame. The current two-ray strata comparator is close but not identical; these
ratios use the explicitly labeled independent baseline. Tile coherence is an
assumption, not established by the R423 image pair.

Therefore do NOT ship this simple per-pixel/screen-tile history guide. It fails
movement cases before any game test. No user capture is needed for that rejection.

## What a viable implementation still needs

1. A proposal keyed/reprojected to the actual receiver and directional frame,
   with explicit camera-cut, disocclusion, orientation and changing-light policy.
   A surface-following guide is a candidate, not a proven solution; lighting can
   change even for a perfectly tracked surface.
2. Frozen proposal identity shared by producer/evaluator, per-ray PDF accounting,
   unchanged fresh visibility/range support and miss contribution. Never choose
   whether to undo guiding after observing a sample without deriving the weights.
3. Learning from the actual per-ray contributions with density correction,
   plus tests for heterogeneous tiles and changing light/occluder conditions.
4. D3D12 bindings/lifetime tests, GPU cost measurement, native ray-count parity,
   and quality checks on the already-accepted desk/plant/projector scenes.

These are engineering gaps, not a request to repeat previous user tests.
No runtime change, new downloadable game build, in-game variance reduction or
FPS gain is claimed.
