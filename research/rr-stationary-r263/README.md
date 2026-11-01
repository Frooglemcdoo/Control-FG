# R263 stationary projector capture

User evidence: the four-pair R263 sequence, PID 37196. All 18 archive hashes
validated before reconstruction; boundary-report.json reports complete. Eight
frames: 5783/5784, 6316/6317, 6842/6843, 7367/7368. This analysis changes no
rendering code and does not produce a replacement game build.

Reproduce with NumPy, SciPy and Pillow:

```sh
python tools/analyze-rr-stationary-r263.py /path/to/restored-capture research/rr-stationary-r263
```

## Observed

- Early GI RGB equals GI-at-RR-input RGB exactly at every pixel in all eight
  frames. Early GI RGB has no NaN/Inf. These copies bracket the bypassed native
  GI filter, not the entire GI producer or every native lighting filter.
- All captured evaluations succeeded with reset=0, Raw Bypass applied, and zero
  observed spatial passes. Pre-exposure is constant at 5.55555534.
- The fixed floor ROI beside the projector is x=[.48,.53), y=[.46,.62) in
  normalized image coordinates. Motion is exactly zero there in all frames;
  raw albedo has no negative missing-material sentinel. Normal reconstruction
  matches the independently captured packed native normal within 0.000489.
  These checks do not prove world/view space, camera matrices or jitter conventions.
- Diffuse lighting in this ROI has the same measured mean and high-frequency RMS
  as early GI; specular lighting is much weaker. The input image already contains
  strong GI noise, while RR initially suppresses most of it.
- During 16.75 seconds, the RR output develops a persistent granular pattern.
  The incoming noise amplitude remains approximately steady. The table uses the
  first frame of each pair; the second frame corroborates each measurement.

| Frame | Seconds after first capture | Input high-pass RMS | RR output high-pass RMS |
|---|---:|---:|---:|
| 5783 | 0.00 | 0.000297 | 0.000016 |
| 6316 | 5.64 | 0.000294 | 0.000038 |
| 6842 | 11.19 | 0.000302 | 0.000061 |
| 7367 | 16.75 | 0.000293 | 0.000063 |

High-pass RMS is luminance minus Gaussian blur, sigma 1 input pixel / 1.5 output
pixels. It includes genuine surface detail and is not a pure noise measure or a
quality score. The matched ROI sequence and visual comparison support the trend;
no general percentage quality improvement is claimed. `floor-sequence.jpg` uses
a fixed 80x display exposure, Reinhard mapping and gamma 1/2.2 to expose the dark
region. Buffers have different physical meanings; cross-column brightness is
not an energy-conservation comparison.

Consecutive-pair early-GI high-pass correlations are approximately -0.025 to
-0.010; output correlations exceed 0.999. Thus the input noise is changing while
the reconstructed pattern is temporally persistent. Frames 5783 and 7367 have
an early-GI correlation of 0.576 despite their separation, suggesting recurrence.
That pair is 1584 frames apart (divisible by 16), but two observations cannot
establish a 16-frame period or identify a faulty seed. Jitter and scene geometry
also affect these measurements. The correlation report uses a wider sigma 2/3
high-pass and unshifted screen coordinates; it is not a motion validation test.

The session log reports effective=6 GI samples around the capture interval.
A separate specular energy clamp remains active at 60%; Raw Bypass is GI-only.
It is not evidence that every native denoiser/filter has been disabled. The
captured floor responsivity mask was independently checked and is zero.

## Interpretation and next implementation gate

The stationary regression is visible in RR output itself; it is not solely a
later presentation/postprocessing artifact. Native GI history/spatial filtering
between the two capture points is ruled out for these frames. Missing material
replay is not the explanation for this sampled floor area. The capture does not
prove that all RR inputs are semantically correct or that the RR model is faulty.

The next source audit should trace the native GI producer's sample phase/seed
and spatial distribution, then verify guide/camera/jitter alignment at the RR
boundary. Existing inputs include the native renderer and rt_diffusegi.obj /
rt_deferred_diffusegi.obj in the supplied loose-shader archive. Do not change seed
constants based only on the observed recurrence. Do not increase sample count,
add history resets, restore native temporal filtering, alter reflection clamps,
or change DirectStorage based on this capture.

A correction build requires an identified contract/sampling defect or a clearly
isolated diagnostic hypothesis. This evidence narrows that investigation but
is not sufficient to claim a root-cause fix.
