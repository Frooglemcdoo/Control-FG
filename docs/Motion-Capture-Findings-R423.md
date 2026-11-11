# R423 moving character capture findings

Capture: GI-Motion-R423-11180-331728828.zip
SHA256: 94e11f645a9855b8916e0db0d0253e32ee8d78e4ec0d2da92785641f93833a32
Frames 12238/12239, 2560x1440 inputs and 3840x2160 RR output.
Both RR evaluations succeed; reset=0; full-resolution pair is consecutive.
All buffer byte counts match metadata. GI RGB is unchanged between pre-filter
and RR input. Motion in the inspected ROIs is approximately 2.2–3.1 pixels.

## Reconstruction-map result

Both maps are 1280x720 R32_UINT, all 921600 words equal 0xffaa5500.
For sample zero the four pixel parities select 0,1,2,3 respectively:
identity mapping. It is identical between the captured frames. This weakens
changing/wrong-center map selection as an explanation for this run.
It does not establish map lifetime at each character draw or coverage.
The zero-coverage sentinel would select 3,0,1,2; the existing coverage guard
must remain. Do not delete the guard based on the uniform map.

## Character signal

At hair pixel (347,632), frame 12238:
- Input RGB: [0.07006836,0.03610229,0.01840210].
- Diffuse lighting: [0.31005859,0.546875,0.38012695].
- Diffuse albedo: [0.22497559,0.06567383,0.04815674].
- Their product accounts for 99.48–99.55% of input RGB.
- Specular lighting is approximately [0.00026989,0.00018036,0.00008839].
- Raw GI luminance is 117.7 times its 5x5 local median.

The selected hair peak in frame 12239 has a 169.9x local GI ratio.
The frame-12238 neck-region peak has a 425x ratio, but this is spatial
contrast, not proof of a physically invalid sample or an unbiased reference.
Do not apply these ratios as clamp thresholds. ROI rectangles are manually
chosen from the image, not verified actor segmentation.

## Captured-input replay

The production R421/R423 emitted filter was executed against the captured
native GBuffer, lighting and reconstruction map, using sample zero.
Six selected input-red-residual peaks (hair, neck and arm in both frames)
each read eight material-compatible lighting neighbors. Every neighbor gets
zero radiance-gate weight. The center survives unchanged at all six points.
An independent existing oracle agrees with the emitted-instruction replay.

This extends the prior synthetic impulse test to captured inputs.
It does not reproduce per-draw buffer timing or RR's temporal history.
Removing this gate repeats rejected R422; it is not the next correction.

## Reconciliation with older work

R275/R278 already audited near-light terms and native accumulation.
They showed concentrated contributions but not an arithmetic defect; R278
native sums agree within floating-point rounding and its fallback must remain.
R419 audited current Owned sampling arithmetic and orthonormal basis.
Do not restart those audits or assert a missing PDF without contrary evidence.
R423 does not contain per-ray light decomposition and cannot identify an
individual failing emitter, proposal or visibility term.

## Decision

No new rendering build or user capture is justified by this result alone.
Retain R423/R421 rendering and rejected status of R422.
Deprioritize generic sample-remapping changes for this capture; investigate
source-variance reduction with explicit energy, visibility and cost validation.
A proposal must preserve accepted projector/desk/plant results, native ray count,
and material detail. No blanket clamp, broader blur or increased RELAX.
Current evidence narrows the investigation; it is not a completed fix.

Reproduce:
python research/motion-r423/analyze.py <extracted-capture-directory>
python research/motion-r423/replay.py <extracted-capture-directory>
Numerical results are committed beside those scripts.

