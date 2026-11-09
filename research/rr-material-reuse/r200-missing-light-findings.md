# R200: remaining missing-light disagreement

The user reports that R200 helps but some native lighting is still missing.
They intentionally compared MAP GUARD ON and OFF. The session log confirms
both variants were prepared and the guard was used. The supplied trace itself
records `native_pcf_blend_r199`, `map_guard=0`; this is a valid baseline capture,
not a user setup error or evidence that the ON result failed to capture correctly.
The two comparison screenshots use different viewpoints and cannot support a
pixelwise brightness or performance comparison.

Inputs: `lighting(20261028-175700).log`, matching manifest and three blocker
PNGs, `Lighting-R200-20261028-124104-72312-0.log`, and screenshots 125016/125044.
The trace completed with zero failures, RR ON in all three images, SPEC Native,
RT effects 0x6B, compact hit distance OFF, normal camera. Strict decoding accepts
4096 valid paired records at 2560x1440, capture generation/frame tag 1604996.

## Quantitative result

The original extreme threshold found 14 samples with native map visibility
above 0.9 and filtered RT below 0.1. All 14 reference rays hit geometry inside
the authored clip segment. There are also 68 opposite extreme samples (native
below 0.1, RT above 0.9).

Using a stated broader threshold, `native map - filtered RT > 0.25`:

- 511 samples meet the threshold; 498 select capture-local spotlight 23.
- 463 reference rays hit geometry; all 463 hits are within authored clipping.
- 48 reference rays miss. Of those, 14 forced-opaque same-mask probes hit;
  34 miss both opaque triangle probes. These auxiliary probes cannot explain
  the individual gameplay ray outcomes.
- 465 samples have partial native visibility between 0.1 and 0.9. Most of the
  discrepancy is therefore outside the earlier near-binary comparison.
- Hit distances span approximately 0.0083 to 8.9854 engine units. A single
  near-origin bias adjustment would not explain this distribution.

The largest hit group among those samples is light 23 / instance 1712 /
geometry 0: 173 hits. Its sampled world bounds are approximately
`[-95.7415,22.3703,31.8053]` to `[-94.9822,24.2784,31.8369]`. This overlaps
the thin spatial band observed in R193/R195. It supports spatial continuity,
not asset identification. Other geometry groups contribute too; do not exclude
this capture-local instance ID as a fix.

## Interpretation

R200 uses the minimum of native-map and faded RT visibility. It can prevent
RT-only leaks, but mathematically cannot recover native light blocked by RT.
The data supports investigating both ray/caster agreement and filtering. The
48 reference misses prevent attributing all missing light to the recorded hit
geometry. They do not prove a filter bug: the gameplay path uses four sampled
rays and spatial filtering, while this capture stores a separate full-length
reference segment. Partial map visibility also includes native filtering and
possibly authored fade; it is not proof that a caster should be removed.

Counts are sampled visibility differences, not a measurement of missing lights,
lost radiance, percentage of darkened screen area, or performance. The native
map sidecar and native composition evaluate maps separately. Capture-local light
and instance IDs are never persistent identities.

## Avoid repeating rejected work

R162 already tested the projection-origin ray path; R163 documented that it did
not remove the earlier boundary. MAP PRIORITY was rejected as well. Neither is
revived here. No clamp, SPEC count, global shadow bias, mesh exclusion, minimum
brightness, or gameplay shader change is justified by this analysis.

The missing measurement is the actual gameplay spotlight output before its
spatial filters, paired with the filtered result and existing map visibility
from the same frame/pixel/light. Capture that read-only rather than switching
to raw rendering, which changes the comparison. Any implementation must verify
the raw producer's layout and resource lifetime, preserve native bindings and
outputs, and use the existing asynchronous/fenced capture ownership rules.
No new in-game test is requested until that measurement is implemented.

## Reproduction

`python tools/analyze-missing-light.py <lighting.log> --output <result.json>`
uses the existing strict capture decoder and matched-map validation, then
reports threshold counts, reference hits/misses and capture-local hit groups.
`python tools/test-missing-light-analysis.py` covers strict threshold edges,
reference misses, grouping and invalid payload/threshold rejection.

This checkpoint changes offline analysis only; R200 remains the current build.
