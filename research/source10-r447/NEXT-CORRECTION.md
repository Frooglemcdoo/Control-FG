# Follow-up: correction direction after the ten-ray source capture

## What the source audit actually settles

All 15 selected brightest interior-skin examples have their dominant secondary
hit outside the captured camera view (projection uses the recorded native view
position and captured projection matrix). The strongest eight have positive
receiver and secondary facing cosines. Their secondary normal lengths are near
one; their weights and radiance are finite. This does not prove occlusion or
correct per-light operands, but provides no demonstrated arithmetic failure.

Earlier R270/R275/R281 audits already checked native attenuation and accounting.
Do not claim this is newly discovered, or repeat constant-attenuation tests.
The R447 trace records aggregate spotlight radiance, not individual light values,
so it cannot certify the exact current light/shadow operands.

The current R434 fixed guide is inactive at ten rays. Its two old target bounds
are also far from this capture's dominant secondary region. It is therefore not
a remedy for this capture, but is not causing the recorded ten-ray noise either.

## Implemented offline proposal audit

`proposal_audit.py` divides complete source receivers into deterministic training
and held-out sets. It builds two world-space target spheres from weighted lit
secondary positions in the training half, without scene coordinates or material
IDs. It evaluates a 90% cosine / 10% cone mixture on held-out skin directions,
with the matching p/q ratio. All radiance, including misses, participates.

For 14,553 held-out interior-skin receivers:

| Proposal | Relative independent-draw second-moment proxy |
| --- | ---: |
| Original cosine model | 1.0000 |
| Old R434 fixed bounds | 1.1110 |
| Automatically discovered world bounds | 0.2638 |
| Discovered far bound only | 0.2267 |
| Deliberately stale, translated bounds | 1.1098 |

These are NOT measured image variance reductions or gameplay performance.
The density is approximated from captured receiver normals and ray endpoints;
actual ten-ray correlations, range/direction correlations, new visibility and
future-frame motion are not measured. Spatial holdout is not temporal validation.
The two-bound model includes one sphere around Jesse; for some receivers that
becomes a full-sphere proposal. The existing sampler supports this but it wastes
some rays. The far-bound-only result is an ablation, not a validated selection rule.
Do not ship these bounds as a new fixed-scene fix.

The 90% original-density floor preserves support and bounds proposal-induced
second-moment inflation by 1/0.9 under the assumed exact density. It does not
bound image variance ratios or guarantee less visible noise with stale targets.
The stale test demonstrates the expected loss of benefit, not a cure for motion.

## Why not another clamp

`geometry_audit.py` additionally applies offline luminance caps to the same
interior-skin mask. Caps at 2x/4x/10x the local 5x5 median discard respectively
3.70%/2.77%/1.46% of its summed GI luminance. This is one frame, not true expected
illumination; it still demonstrates an energy change rather than a sampling fix.
No clamping, blur or temporal history change is enabled.

## Concrete implementation gate

The candidate is automatically learned world-space directional guiding with fresh
rays, not reuse of stale endpoint illumination. Before a gameplay package:

1. Build a bounded GPU target list from valid current-frame secondary samples;
   retain target bounds only, not radiance as the next frame's final lighting.
2. Freeze/version that list for a later dispatch. Producer and evaluator must use
   exactly the same list, per-ray random coordinates and mixture density.
3. Preserve the native independent range draw and use fresh traversal, visibility
   and secondary shading. Weight the complete per-ray RGB, including misses,
   before summing; preserve native hit-distance conventions.
4. Use support-preserving fallback on missing/invalid targets; handle targets
   behind surfaces, receivers inside bounds, camera cuts and scene changes.
5. Validate learning and weighting against moving blockers, moved/destroyed lights,
   heterogeneous normals and disocclusions before another user run. A screen-tile
   history guide failed these tests previously and must not be revived as a fix.
6. Measure target-building, synchronization and shading cost. No extra rays are
   required by the proposal design, but zero extra cost has not been established.

No new runtime rendering implementation or release is claimed by this checkpoint.
