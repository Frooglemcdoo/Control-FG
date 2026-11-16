# R447 expanded source capture: jaw and neck

Input: user Drive folder `1c1WmZNEeH0zdKlmWLuskTUA7caoqrFdw`, manifest plus nine parts.
All archive hashes and chunk hashes matched; all 42 reconstructed files are complete.
Source frame 7831, PID 80292; paired RR frames 7831 and 7832. Input 2560x1440,
output 3840x2160. Source ROI (256,464,512,512) includes the visible neck and jaw.

## Verified source arithmetic

262,144 pixels each have ten ray records and a final summary: 2,883,584 records,
no overflow, no missing ray indices or frame mismatch. Every throughput product,
running sum, final ten-ray average, and lighting-component sum was independently
checked using the full binary capture. Maximum running-sum error 2.38e-7;
final-average error 1.49e-8. Source output agrees with half-float GI to 0.0001191
maximum absolute error. Pre-filter GI RGB equals GI at RR input exactly.

## Interior skin findings

Use raw-albedo tag 1 eroded by three pixels within the ROI (29,154 pixels).
Select source GI luminance >0.001 and >10 times its 5x5 neighborhood median.
This yields 50 strong spikes; it is a defined subset, not a count of every visible dot.

- 48/50 receive more than half their energy from the secondary spotlight component.
- Median spotlight fraction: 86.94%; 90th percentile: 93.08%.
- Median dominant single-ray fraction: 95.57%; 90th percentile: 97.89%.
- Strongest selected skin pixel (582,919): 183.76 times neighborhood median,
  94.60% spotlight contribution, 99.34% from one ray (ray index 4).
- Its dominant secondary surface is material 1913 in this frame, world position
  (-82.01025,25.01980,107.10120). The five brightest examples hit material 1913
  within a small world-space region. These IDs are frame-local, not persistent identity.

This confirms the rare-bright-secondary-hit mechanism on jaw/neck skin, not only
hair. Ten-ray execution and averaging are not the failure in this recorded frame.
It does not prove that spotlight intensity is erroneous, identify an individual
spotlight, or establish that clipping those rays preserves legitimate illumination.
The recorder captures aggregate spotlight lighting, not per-light operands.

## Motion follow-up

Interior neck guide correspondence is best among the tested conventions with
current-to-previous vectors plus previous-minus-current jitter: albedo MAE .003505
versus .004548 unwarped, median normal dot .999943. Hand MAE .001870 versus
.005537 unwarped. This does not support a sign flip. Two-frame agreement cannot
certify all animation, disocclusions, or RR history behavior.

## Evaluator audit and next scope

The existing secondary spotlight evaluator (native dwords 3822-4998) traverses
lights, samples shadow/projection atlases, applies bounded attenuation, and applies
a diffuse cosine/Fresnel energy term. The captured spotlight split is taken at
4982 and material-weighted at 4999. A large finite contribution alone is not proof
of an arithmetic defect. Inspect secondary illumination/sampling variance and how
RR retains these skin outliers; do not change the ten-ray divisor, globally dim
spotlights, or apply another blanket blur based on this capture.

No rendering change or performance improvement is claimed. Analysis scripts and
measurements are retained beside this file. Raw captures remain outside the repo.
