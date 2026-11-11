# Surface-guidance motion audit after R423

Research only; production rendering remains R423. This follows the rejected
simple history-guiding trial rather than reopening RR filtering.

## New measured constraint from the R423 pair

The best tested motion/jitter fit reduces albedo MAE in the hair ROI from 0.02250
to 0.01216, neck from 0.03059 to 0.01715, and arm from 0.01872 to 0.00294.
Motion sign + with prior-minus-current jitter is the best of tested candidates.
This is fit evidence, not exact surface correspondence or validated camera matrices.

Native GBuffer material IDs are unsuitable as persistent keys here. The dominant
hair ID in frame 12238 is 985 and in 12239 is 986. After the fitted reprojection,
only 1.58% of the hair rectangle, 0% of the neck rectangle and 24.6% of the arm
rectangle have equal material IDs. Replay tag signs agree for approximately
98.75%, 95.59% and 99.88%, respectively. Quantitative transitions are in
research/motion-r423/correspondence.json.

These are manually selected ROIs, not verified object segmentation. IDs changing
in corresponding regions do not prove precise per-fragment identity or explain
the current noise. They do invalidate assuming raw IDs are persistent surface
identifiers for new guide history. Prior R305 already warned against treating
geometry addresses/material IDs as stable identity; this is additional image-pair
evidence, not a claim that no earlier work considered material renumbering.

Current RR guide generation observes MaterialDataPart1 each frame and creates
that frame's bounded descriptor (rr_live_guides.h). The existing character
filter compares IDs within the same frame. Neither is changed: this finding
is not proof of a stale RR material-table bug or an incorrect current filter.
The retired TemporalGI receiver key used GBuffer1.B, not this material ID;
do not misattribute its rejected behavior to this new observation.

Raw producer normals and filtered RR hair normals differ noticeably. In the
hair ROI, their median dot is 0.9819, and 10th percentile is 0.8777. That is
consistent with the explicit existing hair-normal smoothing, not a new packing
error. New ray guidance must use the producer's true directional frame.
Normal comparisons in this audit remain in view space: R423 does not export
the current/previous view-to-world matrices, so they are not used to certify
world-space orientation or receiver visibility. No new capture is requested.

## Implemented world-directional reference

src/gi_reuse/world_directional_guide.h adds a research-only proposal over a
world-fixed equal-area sphere (azimuth and z), mixed with the current receiver's
cosine-weighted hemisphere. The guide density is 64*p(cell)/(4*pi).
The current cosine density is max(dot(normal,direction),0)/pi.
Every fresh surface/miss radiance contribution receives p_cosine/p_mixture.
The proposal remains frozen before sampling. There is no cached endpoint or
cached radiance substitution, and the native independent finite-range law remains.

The mixture uses at most 50% guiding. The contribution weight is bounded by
1/(1-guiding_fraction) on the positive hemisphere. Guide samples below that
hemisphere are zero contributions, still counted in the original denominator.
Retrying them, dividing by surviving rays, or replacing them with an unweighted
native sample would change the estimator. Native hit-distance handling remains
an explicitly unresolved integration contract, separate from radiance weights.

This removes a coordinate-frame assumption from the prototype. It does NOT
solve receiver reprojection, guide illumination staleness, disocclusion or the
previously demonstrated learning failures.

## Checks

Built the C++ reference with -std=c++17 -O2 -Wall -Wextra -Werror.
test-gi-world-directional-guide.cpp executes 7.5 million draws across five
normals (including opposite hemispheres) and guiding fractions 0, 0.125 and 0.5.
Constant and linear directional fields match their analytic cosine integrals
within stated Monte Carlo tolerances. Direction lengths, validity, support and
weight bounds are checked. Results include below-hemisphere zero samples.
Raw output is research/directional-guiding/world-proposal-test.txt.

No HLSL compiler, WARP dispatch, live bindings, actual rays, production guide
learning, RR history, visual comparison or GPU timing was executed by these
CPU checks. Neither research header is included by the production path.

Next implementation gate: coherent receiver history and learning must be tested
with world-space orientation, renumbered material IDs, actual motion and changing
illumination. Do not promote this reference into a user build solely because
its estimator integrals are correct. The rejected screen-history candidate and
R422 stay rejected. No generic user capture request or FPS claim follows.
