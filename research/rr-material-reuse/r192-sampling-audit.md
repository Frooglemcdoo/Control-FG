# R192 sampling audit (research only)

This branch starts from delivered R191, commit
74b371b8be84e06ede59bb6067a76798317bf81e. No rendering change or new build is
being claimed. Production and releases are outside this work.

## Evidence and continuity

The previous R191 analysis isolated environment-volume specular as the broad
stationary painting/elevator contribution. Elevator emission also produced
sparse bright points. Bypassing the later RR boundary clamp did not fix the
pattern. These are carried-forward findings: the earlier raw capture batches
and unpushed analysis commit are not present in this restored workspace.
Do not describe this audit as a fresh reanalysis of those images.

Closed experiments remain closed: R179 distinct fallback, R185 phase precision,
R189 volume roughness blend, and the earlier temporal/spatial filtering trials.

## Native sampling facts

The pinned raygen reads sys_constants register 69.y (byte 1108), not the
engine's logged frame number. Primary index is temporal_counter * SPEC + ray
index. Its first two fractional phases select azimuth and a GGX NDF half-vector.
The radial CDF is restricted to [0, 0.4), with alpha = roughness squared.
This is normal-distribution sampling, not a view-conditioned visible-normal
sampler. Changing it to VNDF without auditing estimator weights would change
more than sample coverage; do not ship such a substitution blindly.

The blue-noise texel is selected using a world-space-derived hash and projected
coordinates. The hash construction inspected here does not use the temporal
counter. A stable surface therefore can retain its spatial offset while the
fractional phase changes. This is a design fact, not proof of a defect.

Retries use counter + 10 * retry_number and share their eventual fallback
within a pixel. R176 memoization preserves that original behavior; R179 already
tested a distinct retry strategy without fixing the visual problem.

## A useful exclusion bound

For native radial variable u < 0.4,

    tan(theta_max) = roughness^2 * sqrt(0.4 / 0.6)
    minimum N dot L = cos(view_angle + 2 * theta_max)

The shader rejects N dot L < 0.005. Ignoring floating-point rounding at the
boundary, rejection is impossible below these view angles from the normal:

| Prior ROI roughness | View angle below which rejection is excluded |
| --- | --- |
| Elevator median 0.31763 | 80.30 degrees |
| Painting median 0.35669 | 77.85 degrees |
| Painting upper approximate range 0.46 | 70.11 degrees |

This makes another fallback experiment low priority for broadly front-facing
surfaces. Actual ROI view angles have not been measured here, so this is a
conditional exclusion, not a measurement of zero rejected rays in the captures.
The audit checks source operations and independently verifies the angular
identity using direct half-vector reflection for five viewing angles per
roughness value. It is not a GPU or image-quality test.

## Remaining measurement before changing the sequence

A counter cycling through P values has at most P * SPEC primary indices per
cycle. Raising SPEC increases that finite support, which is consistent with
(but does not explain or prove) the reported SPEC16 improvement. No observed
counter period is available; never substitute logged engine frames for it.
The JSON contains illustrative support bounds, not game measurements.

Next instrumentation must observe the actual producer-bound 69.y value over
at least 256 consecutive successful reflection frames, together with engine
frame, sample count and continuity/reset markers. It must distinguish repeated
values, short cycles, skipped frames and restarts. Reading an unverified CPU
field or borrowing the resolve dispatch's constants is insufficient evidence
of the raygen input. If GPU readback is necessary, use bounded asynchronous
storage and existing fence retirement, with no render-thread GPU wait.

Decision rule: if short cycles are observed, test a longer bounded sequence
while retaining native mapping, weights, count and lighting. If the counter
advances with adequate coverage, stop pursuing counter cycling and investigate
sampling/reconstruction with that hypothesis closed. Neither outcome justifies
repeating R185, whose float-phase precision change was already rejected.

Reproduce:

    python tools/audit-spec-temporal-coverage.py --output research/rr-material-reuse/r192-sampling-audit.json
