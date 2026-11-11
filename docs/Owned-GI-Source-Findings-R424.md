# R424: hair and jaw source attribution

2026-11-11. Analysis checkpoint, not a rendering change or a completed fix.

## Capture contract

Hair: PID 76708, source/RR frame 8798, following RR frame 8799,
ROI [409,448,256,256]. Jaw: PID 32388, source/RR frame 4029,
following RR frame 4030, ROI [947,592,256,256]. Both inputs are
2560x1440; reconstructed outputs are 3840x2160.

All five parts of each upload, chunk hashes and contiguous coverage verified.
Each capture reconstructs 42 files. Both traces are complete, without overflow,
and joined to the first RR frame. Each contains 65,536 receivers and 131,072
records: one ray and one summary per receiver. Actual configured ray count is
ONE in these runs; older two-ray assumptions do not apply.

Each capture has only ONE source frame. The two RR frames do not provide a
second source decomposition. Spatial contrasts below are not temporal variance
measurements and cannot establish RR history correctness.

## Source-to-consumer agreement

Pre-filter GI and GI at RR input match exactly. Float32 source output differs
from stored half-float GI by at most one half-float ULP from nearest rounding;
all source channels are greater than or equal to the stored channels. This is
consistent with downward quantization, not an identified transfer corruption.
Do not describe the source float32 comparison as bit-exact.

## Verified character peaks

Hair segmentation uses the private replay alpha tag [-3,-2] plus valid RGB
[0,1]. All 58,070 tagged receivers in this ROI have valid RGB. Alpha<0 alone
also admits the -4 background sentinel and must not be used as a hair mask.
The jaw region is a manual box [1045,1155)x[700,790) intersected with positive
replay alpha, containing 4,751 receivers. This is not general actor segmentation.

| Region / pixel | Spotlight fraction of GI luminance | Secondary material | Hit distance | GI / local median |
|---|---:|---:|---:|---:|
| Hair (650,561) | 95.41% | 107 | 0.96965 | 789.46 |
| Hair (594,554) | 96.14% | 107 | 1.12629 | 357.69 |
| Jaw (1053,703) | 92.96% | 727 | 0.12189 | 56.40 |
| Jaw (1080,713) | 92.29% | 727 | 0.10991 | 23.98 |

Point lighting supplies about 4-5% at these hair peaks; volume supplies most
of the remaining jaw contribution and point lighting is zero there. Component
sums agree with the native secondary result within floating-point rounding.
Primary and secondary facing cosines are positive at these examined peaks.
The final diffuse-times-albedo term accounts for most of the incoming color.

Material IDs are per-frame keys, not stable object identities. Neither material
107 nor 727 is positively identified as a particular prop or body surface.
Distances are native world units. Large spatial contrast does not establish
an invalid sample, a physical lighting error, or a safe clamp threshold.

## New sampling concentration evidence

In the hair ROI, just 32 of 58,070 tagged primary receivers hit lit secondary
material 107 (0.0551%), yet those samples supply 7.567% of the region's summed
GI luminance. In the jaw box, 158 of 4,751 receivers hit lit material 727
(3.326%), supplying 25.851% of summed GI luminance. These are spatial counts
in one frame, not probabilities for an individual receiver over time.

This matters for the next design: merely smoothing illumination evaluated at
the secondary hit does not necessarily solve rare bright-direction sampling.
Even a perfectly stable bright secondary surface can produce large primary
pixel fluctuations when a one-ray estimate alternates between that surface and
other directions. The captures support investigating this mechanism; they do
not establish it as the sole cause of the observed motion artifacts.

Do not generalize the selected 95-96% hair spotlight fractions to all hair.
For the brightest 1% of tagged hair samples, aggregate components are about
67.84% spot, 31.50% point, 0.66% volume. Whole-region miss/volume energy is also
substantial. Removing spot or volume lighting would remove real energy.

## Implementation audit and decision

`tools/character_diffuse.py` filters the already-combined diffuse input t3.
There is no separately bound incoming GI texture in that character filter.
Removing its radiance gate repeats rejected R422 and can disturb direct-light
detail. Do not apply that change again.

`gi_owned_lighting.hlsl` has the separated incoming GI before composition,
but its current bindings do not contain the replay character tag. Do not
hardcode the material IDs above as character identity. The existing owned
spatial filter is already an older option, not a newly discovered correction.

R416's selectively replaced endpoint history was biased; it must remain retired.
The previous tile guiding experiment is also not validated by these captures.

Next implementation gate: test a properly weighted sampling/reuse proposal
against rare bright secondary surfaces, nearby occlusion, heterogeneous receiver
normals and moving geometry. Preserve proposal support, visibility and energy;
budget actual ray/evaluator work. A source-radiance cache alone needs proof that
it reduces primary variance, rather than merely reusing stable endpoint values.
No new global blur, brighter-sample suppression, ray increase or RELAX change
is justified by these measurements. No performance claim is made.

## Research cross-check

NVIDIA's ReSTIR GI integration treats secondary surfaces as samples and requires
application bridge functions, target evaluation and visibility handling. It is
an architectural candidate, not a drop-in RR option or a demonstrated FPS gain:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md

NRD recommends material demodulation for its denoised signals. That supports
keeping material detail outside an illumination filter, but NRD guidance is
not a specification of RR's internal behavior:
https://github.com/NVIDIA-RTX/NRD

## Reproduction

Small numeric results and analysis scripts are in
`research/owned-source-r424/captures/`. Extract the uploads into sibling `hair`
and `jaw` directories, then run `analyze.py <capture_root>` followed by
`geometry.py <capture_root>`. Requires NumPy and SciPy. Scripts write intermediate
arrays and JSON beside those inputs, not into the repository. Raw capture
buffers are intentionally excluded from git.
