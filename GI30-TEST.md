# GI30 — Clean RenoDX RR Baseline

GI30 isolates the RR evaluation baseline before any further denoiser experiments.

## Baseline state

- Reflection clamp remains **60%**.
- Specular motion vectors are hard-disabled at the evaluation boundary.
- `GBuffer.SpecularMvec` is cleared every RR evaluation.
- `MotionVectorsReflection` is cleared every RR evaluation.
- `DLSSD.SpecularHitDistance` is cleared every RR evaluation.
- `WorldToViewMatrix` is identity.
- `ViewToClipMatrix` is identity for both Model E and Model F.
- The GI29 six-shader Direct RenoDX DLF replacement remains available for A/B, but defaults **OFF**.
- Contact-shadow filtering remains untouched.

This intentionally removes the GI27 specular-MV, D1 hit-distance, and P1 projection experiments from the active RR path.

## Test

1. Start with **Direct RenoDX DLF shaders = OFF** and reproduce the shadow-boiling scene.
2. Without changing any other RR setting, switch **Direct RenoDX DLF shaders = ON**.
3. Compare the same camera movement and stationary view.

Expected GI30 markers include:

- `RR_GI30_RENODX_BASELINE ... specular_mvec=cleared reflection_mvec=cleared hit_distance=cleared matrices=identity`
- `RR_GI30_GEOMETRY_BIND ... matrix_mode=identity`
- `RR_NATIVE_EVALUATED ... matrix_mode=identity_renodx_baseline`
- Existing GI29 DLF capture/set markers when Direct DLF is enabled.
