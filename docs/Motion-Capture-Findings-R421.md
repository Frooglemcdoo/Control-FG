# R421: measured jaw and hair motion captures

2026-11-11. Analysis only. R420 did not fix either artifact; no new runtime changes or claimed fix.

## Inputs and validity

The user identified the first capture as neck/jaw and the second as hair.

| Capture | Frames | ZIP SHA256 |
|---|---|---|
| GI-Motion-R421-55400-296199140.zip | 8685, 8686 | 4185e058c35c471469cd1a84f467b4646a22052210210cebcf51b6b40255bd53 |
| GI-Motion-R421-55400-296296531.zip | 19346, 19347 | 4809e661b03443ac94157bf2771093e6668a9942863424bd23cfe58a3b5f6246 |

All four frames: 2560x1440 input, 3840x2160 output, stride one, RR result 1, reset 0, raw bypass, zero spatial passes. Each pair is consecutive. Pre-GI and RR-bound GI match over all 3,686,400 RGB pixels according to capture counters. Exposure is unchanged. These are two frames per location, not a convergence study or a GPU performance measurement.

## New pixel-level evidence

Hair interior ROI uses input coordinates [250,460) to [560,680); tight jaw ROI [980,680) to [1020,705). Coordinates are zero-based. Local medians are 5x5; values are linear buffer data, before display tonemapping.

| Measurement | Hair 19346 | Hair 19347 | Jaw 8685 | Jaw 8686 |
|---|---:|---:|---:|---:|
| Maximum raw-GI luminance / its local median | 172.84 | 163.94 | 34.69 | 84.01 |
| ROI pixels >10x local GI median | 0.913% | 0.912% | 18.0% | 17.0% |
| Mean raw-GI luminance | 0.005556 | 0.005569 | 0.043067 | 0.043212 |

The maximum pixel is selected independently in each frame; this is not a tracked-pixel ratio. Nearly unchanged ROI means do not prove unbiased sampling or energy conservation.

At hair frame 19346, input pixel (389,504):

- Raw GI RGB: [1.473633, 1.144531, 0.702637].
- Accumulated diffuse RGB: [0.365479, 0.647461, 0.459229].
- Diffuse albedo: [0.202148, 0.052795, 0.036682].
- Input color: [0.073853, 0.034180, 0.016846].
- Accumulated specular RGB: [0.00004983, 0.00003475, 0.00001687].
- Exact hair tag -3 (full weight), normal length approximately one, ordinary finite depth.

Diffuse lighting multiplied by hair albedo approximates this bright input pixel to within 0.1% per channel. This is a concrete diffuse-light spike colored by the hair material, not a demonstrated red corruption of the material guide. The strongest local output red peak lies nearby at output (583,757), input equivalent approximately (388,504). RR has reduced the spike substantially, but the output image still has visible red points. Do not infer a one-to-one RR mapping from these nearby coordinates: RR uses history and spatial reconstruction.

The jaw's bright pixel (990,683), frame 8685, likewise has raw GI [0.442871,0.236450,0.135132], accumulated diffuse [0.306396,0.206421,0.124634], and very small accumulated specular (~0.00004). The guide contains normal skin reflectance. A larger jaw ROI [980,680) to [1032,711) gives local input-luminance-residual correlation with diffuse lighting 0.978/0.981 and with specular 0.136/0.072. Hair diffuse correlations are 0.850/0.848; specular 0.248/0.267. These are spatial correlations, not causal percentages.

## Motion and guides

Warping the previous diffuse guide with current normalized motion times [2560,1440], plus previous-minus-current NGX jitter, gave the lowest error among the tested signs/jitter conventions in both ROIs. Hair guide MAE was 0.01063, versus ~0.0227 without motion; jaw was 0.01032 versus 0.01521 without motion/jitter. Hair motion was roughly (+2.44,-7.70) input pixels at the median, jaw (+1.86,+0.10). This argues against a simple reversed-vector explanation; it does not validate every disoccluded or transparent pixel or the runtime feature flags.

Inspected guides are finite; normal lengths in the tight ROIs are ~0.9992 to 1.0. Negative raw-albedo alpha is the intentional hair tag, not negative RGB radiance. Existing hair responsivity remains in [-0.75,0]. No recommendation to zero or dilate guides follows from these measurements.

## Decision

Both sites now have full-resolution evidence of large diffuse-input fluctuations. The existing R360 filter's proven impulse-preservation behavior is relevant to the observed signal, but these captures still do not expose the native packed reconstruction map or per-fragment coverage. Therefore they do not establish that any particular mapped source is invalid, nor that the underlying Monte Carlo sample is physically incorrect.

The next correction should address diffuse sample variance and its character reconstruction while retaining lighting energy and material detail. First replay candidate reconstruction on these captured regions, with destination depth/normal/material constraints and explicit matched-surface/edge cases. A raw median clamp would remove substantial jaw illumination (17-18% of pixels exceed 10x local median here) and is not an acceptable fix. If a mapping defect cannot be demonstrated, pursue source sampling/importance improvements with correct weights rather than a global blur, RELAX increase, or another history-reset tweak. No new user capture is requested from this analysis.

## Reproduction

Extract the two ZIPs into a local directory containing `jaw/rr-input-c1-55400-8685`, `jaw/rr-input-c1-55400-8686`, `hair/rr-input-c1-55400-19346`, and `hair/rr-input-c1-55400-19347`.

Set `CONTROL_R421_CAPTURE_ROOT` to that directory. Run `research/motion-r421/peaks.py`, `measure.py`, and `motion_check.py` with Python, NumPy, SciPy and Pillow. Text results are retained in that directory in the repository; render_buffers.py recreates the labeled crops locally. Crop previews use identical exposure multiplier 8 and x/(1+x) display mapping; they are diagnostic previews, not game tonemapping.

NVIDIA's RR integration guide describes noisy color, diffuse/specular reflectance guides, normalized shading normals and dense motion vectors as separate inputs. Their presence does not guarantee temporal convergence: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md
