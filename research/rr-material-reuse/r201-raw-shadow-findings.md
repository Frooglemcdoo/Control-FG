# R201: raw-versus-filtered missing-light result

Inputs: lighting(20261028-184421).log, manifest(20261028-184419).txt,
Lighting-R201-20261028-133232-70096-0.log and three blocker PNGs.
Native Blend / MAP GUARD OFF, RR ON in all PNGs, SPEC Native, hit distance
OFF, RT effects 0x6B, normal camera. Capture completes with zero failures.
The strict decoder accepts all 4096 ray/map/raw/filtered samples. Native raw
observer validation passes in-game: zero invalid records. Ray/map generation
1223543 and composition generation 1223545 share captured present 40146.

## Result

At the stated threshold native-map visibility minus filtered RT > 0.25:

- 302 samples across all selected lights; 279 have raw visibility zero and
  23 have raw visibility one. All raw records are present and valid.
- The affected spotlight (capture-local ID 21) accounts for 289 samples.
  Its emitter is approximately [-94.2323304,24.5059052,36.5779421], matching
  the physical light in prior captures. Flags are 257, native fade is 1.
- Of those 289, 266 are fully blocked before filtering and have reference
  hits within the authored clip segment. The remaining 23 are fully lit
  before filtering, have reference misses, and filter down to approximately
  0.143 through 0.740. These categories do not overlap for this capture.
- All seven extreme native-lit (>0.9) / filtered-blocked (<0.1) samples
  across the capture already have raw visibility zero. Six select light 21.
- 285 of the broader 302 samples have partial native-map visibility.
- There are 77 samples with the opposite extreme disagreement (map blocked,
  RT lit). Map guard remains relevant to that separate direction.

The dominant reference-hit group among the affected samples is instance 1381,
geometry 0, light 21, with 140 hits. Sampled bounds are approximately
[-95.7415,22.6037,31.7832] to [-94.9259,24.1477,31.8470], again overlapping
previous captures' thin band. These are local identities, not an identified
asset/material or a safe exclusion key.

The remaining 13 threshold samples involve other lights. Some have native
fade below one; do not interpret their map-versus-unfaded-RT difference as
missing final radiance without applying the layer-specific composition formula.
Do not compare 302 here with 511 in R200 as a quality improvement: R201 made
no rendering change, and camera placement/scene content differ.

## Consequences

Filtering measurably darkens 23 initially clear samples, but it is not the
primary source of this sampled spotlight discrepancy: 266/289 are blocked
before it. This also places the main disagreement upstream of RR. Neither
result proves that the RT blocker is incorrect; native shadow-map filtering,
caster representation and authored source geometry may differ.

Source audit confirms the accepted R97 CENTER FILTERED baseline sets all four
spotlight disk radii to zero (`center_sample_probe`). Thus the nominal four
samples coincide; the centered-reuse optimization can reuse the first trace
without changing their result. Do not describe them as four independently
spread area-light samples. Binary raw visibility is expected. The earlier
R201 progress descriptions of a "four-ray" measurement referred to that
aggregation contract and did not establish four independent directions.

R97's documented user result explicitly selected this centered baseline to
fix close-shadow loss. Restoring the old disk spread wholesale would repeat
a rejected configuration. R162 projection-origin rays also failed previously.
The next implementation must distinguish native map shadow footprint/caster
coverage from centered RT coverage while retaining the close-shadow fix;
this evidence does not justify a global brightness floor, blocker exclusion,
SPEC change or another RR/clamp experiment.

No repeat of this capture is needed. The raw observer now supplies the missing
measurement and should remain available for subsequent changes.
