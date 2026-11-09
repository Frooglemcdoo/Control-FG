# Refocus: trace speckles through the existing GI/RR path

This supersedes the immediate sample-reuse implementation direction in
`gi-trace-findings-r275.md` and `gi-runtime-findings-r278.md`. Geometry/instance
work remains available but is paused until a replacement estimator is justified.
No production shader or runtime behavior changes in this audit.

## Evidence already available

The six recovered R271 frames (4578/4579, 5286/5287, 6069/6070) include native
pre-filter GI and GI at the RR input from matching frame IDs. Every frame reports
raw bypass. RGB is bit-identical between these two GI boundaries. Alpha differs
intentionally and is excluded. Thus bypass did not create or amplify their RGB
spatial peaks. This does not clear RR's reconstruction or material-guide semantics.

Inspection of the raw GI and input-color buffers shows isolated bright samples
on the plant and around hair/neck before RR. Display previews used the same
monotonic mapping x/(x+0.05) followed by gamma 1/2.2; quantitative analysis uses
unmodified float values, not previews. These are older captures with reflection
clamping still enabled, not the user's latest R280 output.

`tools/audit_gi_signal_origin.py` checks dimensions, finite data and same-frame
GI metadata, hashes the inspected files, and measures bright spatial samples.
The criterion is GI luminance above eight times a 5x5 median (denominator floor
0.0001) and above 0.001. This is a declared diagnostic threshold, not a proposed
clamp. Tagged hair is eroded by a full 5x5 neighborhood to exclude tag boundaries.

| Frame | Hair-interior GI candidates | Also >2x input-color local median |
| --- | ---: | ---: |
| 4578 | 587 | 400 |
| 4579 | 615 | 384 |
| 5286 | 759 | 551 |
| 5287 | 729 | 532 |
| 6069 | 713 | 536 |
| 6070 | 697 | 498 |

Counts are spatial contrast, NOT erroneous-sample counts, temporal variance,
perceptual severity or proof of an invalid estimator. Legitimate highlights can
pass the thresholds. Hair tags are replay coverage, not named-object labels.
No numeric plant-only claim is made because there is no verified plant mask.
RR output is 3840x2160 whereas input is 2560x1440; direct pixel subtraction would
be invalid. These jittered frames are not compared as aligned temporal samples.

The previous R275 spotlight trace already checked finite values and the native
attenuation/cookie/visibility arithmetic. It exposed concentrated near-light
contributions but did not establish they were wrong. R277-R280 accounting traces
agree with native sums but are separate sessions and cannot be joined to these
pixels. Session-local light 6 cannot be named as the desk lamp from its ID alone.

## Decision

The captured bright samples are present upstream of RR and unchanged by GI raw
bypass. RR may still retain or accentuate them; its contribution remains open.
The user's lamp-removal/indirect-off tests motivate investigating light-dependent
GI, but do not identify the exact failing term. Do not claim a new root cause.

Pause instance/triangle readback and sample reuse. The next focused probe should
capture, in the SAME dispatch and for the same pixels/rays, individual spotlight
contributions alongside total secondary shading and final normalized GI. Include
light identity/position, primary and secondary material, distance/attenuation,
visibility, material response, sample count and depth normalization. Preserve
native arithmetic. Account for skipped branches and misses rather than reporting
them as zero measured values. Use a user-identified affected ROI rather than a
sparse full-screen grid that can miss the speckles. Then correlate with the GI
image and RR boundary by actual frame/generation, without repeating unsafe large
synchronous RR captures. Only this evidence can support selecting a correction.

Validation: four synthetic test methods cover known bright-sample detection,
boundary mismatch, truncated/nonfinite input, and changed bypass RGB. All pass.
Reproduce (requires NumPy and SciPy):
`python tools/audit_gi_signal_origin.py <recovered-root> --output <report.json>`
The committed JSON contains file hashes, thresholds, coordinates and limits.
