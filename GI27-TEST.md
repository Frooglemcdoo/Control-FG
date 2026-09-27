# GI27 — Explicit Specular Motion Vectors A/B

GI27 starts from clean public v2.1.1. It does not carry GI21/GI22 prefilters, GI24 native-denoiser hybrid behavior, GI25 responsivity/frame-time changes, or GI26's NGX hit-distance matrix change.

## Model F A/B

Open **F10 -> RR Settings -> Specular motion vectors**.

- **ON (default in GI27):** generate reflected-image motion vectors from Control's owned reflection hit-array copies, the exact NGX depth/MV inputs, GBuffer1 gloss, and current/previous camera transforms. Bind the result to both `GBuffer.SpecularMvec` and `MotionVectorsReflection`; clear `DLSSD.SpecularHitDistance`.
- **OFF:** public v2.1.1 Model F path. Specular MV aliases are cleared and the optional D1 `DLSSD.SpecularHitDistance` path is used when available.
- Model E does not run GI27 SpecMVs.

The shader follows the recorded RenoDX Control RR reference formula:

`virtualPos = surfacePos + viewDir * effectiveHitDistance`

`specMV = gameMV + (NdcDelta(virtualPos) - NdcDelta(surfacePos)) * MVScaleCorrection`

It intentionally does not project the physical reflected-ray hit with `reflect(viewDir, normal)`.

## Runtime proof

Expected markers:
- `RR_GI27_SPECMV_READY`
- `RR_GI27_SPECMV ... history=...`
- `RR_GI27_SPECMV_BIND ... active=1 ...`
- `RR_NATIVE_EVALUATED ... specular_mvec=<non-null> reflection_mvec=<same non-null> hit_distance=(nil)`

The RR Settings button shows **ON - ACTIVE** only after the explicit MV texture was generated for an evaluation. **ON - FALLBACK** means the current frame used the public hit-distance fallback.

## Test

Use Model F and the same noisy/boiling scene. Compare ON versus OFF. Check both reflection motion and Jesse's moving low-light shadow region. If ON changes reflections but not the shadow boiling, the next investigation should be primary motion/guide correspondence rather than another specular denoising experiment.
