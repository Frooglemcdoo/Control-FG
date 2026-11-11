# Current-frame two-ray candidate — 2026-11-11

Research only. R423 game behavior is unchanged. No live DXR dispatch, image fix,
frame-time improvement or completed ReSTIR implementation is claimed.

Validated research commit: `87eff22a3c6c37e536775188f49cf960c9ae0ab9`.
[Windows run 43026247360](https://github.com/Frooglemcdoo/Control-FG/actions/runs/43026247360),
job 130541563153, passed DXC compilation and WARP execution, including the
current-frame split, GPU resolve, exact native empty-guide fallback, earlier
two-frame checks, and captured-lighting accounting checks.

## Why test a different schedule

The previous history guide can be correctly reprojected and still favor stale
lighting. Correct probability weights preserve energy but do not prevent higher
variance. Rather than assume perfect surface tracking solves that problem, this
candidate removes previous-frame illumination from the proposal entirely.

For the two-ray case only:

1. Draw one native pilot per receiver and evaluate fresh secondary lighting.
2. Construct a proposal from admitted pilots in this frame.
3. Draw one independent fresh ray from that frozen proposal; apply the angular
   native/guided PDF ratio to its surface AND volume-miss contribution.
4. Output half the sum of the unmodified pilot and corrected second contribution.

There are two samples per receiver, not two pilots plus another ray. No cached
endpoint, radiance substitution, cross-frame material ID or blurred final color
is used. Other native ray counts would need their own budget-preserving schedule;
they must not silently be forced to two.

The pilot must NOT be retroactively weighted using the proposal learned from it.
Conditioned on all pilots, the second estimate has expectation I. Therefore its
covariance with the pilot is zero and the pair remains unbiased even when a
receiver's own pilot informs its proposal. The pair's variance is
`(Var(native pilot) + E[Var(second | pilots)]) / 4`. At this budget it cannot
remove the first ray's variance; even a perfect second estimate gives at most
a factor-two per-receiver variance improvement over independent native pairs.

## Toy results and rejection cases

`research/directional-guiding/current_frame_trial.py` fixes seed 426 and runs
8192 pilot sets per configuration. It compares a discrete 64-cell field against
two independent uniform native draws. This is not Control's stratified sampler,
world-sphere mixture, captured lighting directions, or an image-quality test.
The 24 cases vary group size, illumination heterogeneity, and guide strength.

For 128 receivers per group, ratios of per-receiver variance to the independent
two-ray reference are:

| Different lighting fields within the group | Guide fraction 0.125 | Guide fraction 0.5 |
| --- | ---: | ---: |
| 1 | 0.6167 | 0.6342 |
| 2 | 0.7573 | 0.8382 |
| 8 | 0.9612 | 1.2170 |
| 16 | 1.0086 | 1.3131 |

The expected mean is 1.61171875. Conditional mean checks pass. A deliberately
incorrect pilot reweighting gives about 0.9239 in the coherent 0.125 case,
demonstrating the energy error that the separate pilot term prevents.

Interpretation: the approach helps the coherent toy but is NOT safe to enable
over arbitrary tiles. Even the weaker guide slightly worsens the heterogeneous
16-field case. No temporal history does not mean no failure modes. Spatial
correlation and perceived noise are not measured by these variance ratios.

## Existing capture grouping evidence

`research/motion-r423/current_frame_groups.py` counts equal current-frame material
IDs and replay-tag signs inside 8x16 tiles. It never compares IDs across frames.
For both R423 captures the median group size in the selected hair, neck and arm
rectangles is 128. Between 93% and 98.5% of those ROI pixels have at least 64
same-ID/tag members. Full values are in `current-frame-groups.json`.

These are upper bounds before normal/depth rejection. ROI rectangles are not
segmentation; equal IDs can span disconnected geometry and different lighting.
They show available sample population, not validated sharing or visibility.

## GPU prototype

The research WARP fixture has a separate current-frame mode. It makes all 640
pilot proposals native, learns with no old-energy term, publishes for the same
frame, and samples a fresh second direction with new random coordinates.
`LearnCurrent` uses 128 contiguous pilot slots per group, validates identities,
and reduces 64 angular cells in group-shared memory. It does not scan the entire
input for every guide. This bounded layout is a fixture contract; runtime tile
admission and packing are not implemented.

`ResolveCurrent` combines the untouched pilot with the corrected second sample
on GPU and checks the two-term denominator. Current and previous-stage outputs,
frame/generation rejection, empty groups, CPU PDF/contribution parity and memory
guards are checked. The whole sequence submits without intermediate CPU readback.
Fresh lighting is still synthetic, not execution of Control's full evaluator.

An empty guide now explicitly restores native cosine sampling. A uniform
world-sphere table at nonzero guide fraction would still alter the proposal and
admit below-surface zero terms; it is not equivalent to native fallback. A test
uses a guide with no valid pilots, then consumes it in the second stage and
requires unit correction and a valid upper-hemisphere sample. This fixes a
research fallback contract, not a demonstrated bug in the shipped renderer.

An initial CPU/GPU cell equality check failed for a native ray exactly on an
angular-cell boundary. The reference now allows only adjacent bins within
2e-5 bin units, checked against both directions; numerical PDF, direction and
contribution checks remain. Arbitrary cell mismatches are not accepted.

## Shipping gates

Do not enable arbitrary same-material tiles on these results. Normal/depth and
illumination coherence need a defensible admission policy and adversarial tests.
Do not call the bounded reduction fast without hardware measurements.

This schedule adds a dependency between two ray/shading stages. Despite retaining
two rays, it can add dispatch, repeated receiver setup, descriptor and memory
cost. Actual producer/evaluator binding, hit-distance semantics, lifetime, and
RTX timing must be established before a user build. A same-frame guide is a
candidate that avoids one known failure mode, not a proven cure for Jesse.
