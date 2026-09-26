# GI26 — Model F Specular Reprojection Camera Fix

GI26 starts from clean public v2.1.1. GI24 hybrid and GI25 responsivity/frame-time experiments are not carried forward.

The public F path can bind `DLSSD.SpecularHitDistance`. NVIDIA's RR contract says Specular Hit Distance is the alternative to application-provided specular motion vectors, but it must be paired with the real WorldToView and ViewToClip matrices.

The old F path supplied the native ViewToClip matrix but left WorldToView as identity because the material normal guide is view-space. That conflated two independent contracts. GI26 fixes only that mismatch:

- Model F + valid hit distance: validated native WorldToView (expanded from Remedy's row-major 4x3 affine matrix) + validated native ViewToClip.
- Model E: unchanged reference identity matrix path; no SpecularHitDistance.
- Native Control denoisers remain bypassed exactly as in public v2.1.1.
- No prefilter, hybrid denoiser, responsivity mask, frame-time experiment, or GI history modification.

Expected log marker: `RR_GI26_SPECULAR_CAMERA ... valid=1 world_to_view=1 view_to_clip=1 mode=native_world_to_view_plus_projection`.
