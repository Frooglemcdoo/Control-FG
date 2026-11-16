# User-confirmed 10-ray motion capture

Input: `GI-Motion-R424-47484-728168437.zip`, downloaded from the user's Google
Drive link. ZIP CRC verification passed. Frames 8166 and 8167; 2560x1440 inputs,
3840x2160 RR output. The user confirms 10 rays. Metadata does not independently
record the ray count, active producer, or current build; R424 is a capture label.

## Established from these buffers

- RR is active, reset is zero, and the GI boundary is raw bypass in both frames.
- Pre-filter GI and GI at the RR input match exactly in RGB across all 3,686,400
  input pixels in each frame. The intervening GI filter does not generate these dots.
- Inspected buffers contain no NaNs or infinities. This does not establish correct
  values, bindings, motion correspondence, or complete RR integration correctness.
- Visible jaw/neck dots already exist in raw GI and the composed input color.
  At (547,751), frame 8166, raw GI luminance is 78.43 times the 5x5 median;
  diffuse-lighting times diffuse-guide RGB explains 98.6–98.9% of input RGB.
  At (544,788), frame 8167, the ratio is 209.52 and the explained fraction is
  99.3–99.6%. These are selected examples, not a whole-character statistic.
- RR preserves conspicuous dots on the jaw/neck while largely removing sampled
  dots on the hand in this same pair. Visual inspection agrees with the output
  footprint metrics in results.json. This is a new useful internal comparison:
  bright source samples alone do not explain the different reconstruction result.
- Sampled skin normals have approximately unit length and finite roughness.
  No obvious numerical guide corruption was found at these dots.

## Motion and identity limitations

The inherited correspondence search, using these capture-specific rectangles,
reduces neck albedo MAE from 0.03145 to 0.01615 with positive motion and a tested
jitter correction. This is only a best fit among candidate conventions, not proof
of accurate deforming-skin motion. The hand prefers no displacement among those
candidates. Different regions cannot independently choose the actual RR convention.

Neck material IDs change between frames (dominantly 1135 to 707); hand IDs are
mostly stable. Earlier R423 evidence already showed frame-local material index
changes. This does NOT prove a stale material table or a bug, and raw IDs must not
be used as persistent history identities. See docs/Surface-Guidance-Motion-Audit.md.

## Source audit

The complete evaluator reference loops over giCB4[0].z (native dwords 983–992),
loads each hit slice using that loop index (995 onward), accumulates at 5029,
increments at 5038, and divides RGB by the same count at 5143–5150. No skin-specific
one-ray shortcut was found in that loop. This is static code evidence, not proof
that this capture dispatched the expected constants and shader on every pixel.
The R434 fixed guide only changes the one-ray route; R445 progressive coordinates
only change the four-ray route. Neither explains a ten-ray-specific guide effect.

## Decision and remaining evidence

Do not conclude that RR creates the dots, that RR inputs are fully correct, that
all 10 samples definitely ran, or that the problem is simply too few rays.
Do not apply another blanket clamp or spatial blur on this evidence.

The next causal check must explain why reconstruction retains neck spikes while
removing hand spikes: validate deforming-skin motion and the precise RR temporal
input convention, and compare stable material properties/normal changes rather
than frame-local material IDs. If those inputs are correct, inspect the per-ray
contributions that produce the source outliers. This archive has no hit tables,
per-ray radiance, light attribution, or previous world transforms, so it cannot
identify a particular secondary surface/light or prove reprojection correctness.

No rendering code was changed. No new build or performance gain is claimed.
The diagnostic script and machine-readable measurements are retained here.

## Follow-up: interior-skin motion test and R446 recorder

The broad neck rectangle included changing hair/skin boundaries. A refined test
uses the positive skin replay tag, erodes its boundary by three pixels, and checks
the previous tag after reprojection. With current-to-previous motion plus
previous-minus-current jitter, neck albedo MAE drops from 0.010655 to 0.001429
(86.6%); median normal agreement is 0.999989. The hand also agrees well with this
convention (albedo MAE 0.000825). This does not support changing motion sign or
scale. It cannot certify occluded/revealed pixels or the rest of an animation.
The scripts and full metrics are in reprojection.py and reprojection.json.

Skin samples use zero responsivity; nonzero values within the broad neck rectangle
belong to hair coverage. Code forwards the engine motion resource, scale and jitter.
Older creation logs show flags=3 (HDR and low-resolution vectors, no MVJittered);
those older logs are not direct evidence of this capture's feature flags.

R446 repairs a diagnostic gap: the owned source recorder was attached only to the
ordinary evaluator, while GI: FIXED selects the guided evaluator. The new source10
session attaches the recorder to both, requires ten rays, and uses a 112x112 ROI
that fits complete ray sets within existing capacity. The collector independently
requires ten executed rays at every valid receiver. Source records are matched to
the same RR frame. The diagnostic evaluator preserves the original lighting
arithmetic, but does not preserve capture-frame timing. Normal gameplay shaders
are unchanged. Windows build validation is required before delivery.
