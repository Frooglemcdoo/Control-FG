# GI33 — Live Jitter A/B + RT Diffuse Samples 1–16

GI33 removes the crashing GI32 `g_uDGIPassCount` experiment. The DGI pass topology is no longer modified.

## Fixed baseline

- Direct RenoDX DLF shaders: **ON by default**
- Reflection clamp: **60%**
- Specular MV / reflection MV / specular hit distance: cleared
- RR matrices: identity
- Contact-shadow routing unchanged

## Live controls

### Jitter
- **CONTROL**
- **RR 1024**

### RT diffuse samples
- **NATIVE** — preserve Control's `g_uRTDiffuseRayCount`
- **1–16** — every integer selectable

The new hook is the audited `setProviderData` call at renderer RVA `0x12BF07`, before the renderer mirrors the selected value. It no longer touches `g_uDGIPassCount`.

Changing jitter, diffuse samples, or Direct DLF forces an RR history reset on the next RR evaluation.

## Test

Keep Direct DLF ON and Jitter CONTROL. Start at NATIVE. In the same noisy low-light scene, move through diffuse sample values 1 → 16 and identify the lowest value where noise stops improving. Watch FPS/frametime because ray cost should rise sharply.

Expected markers:
- `RR_GI33_DIFFUSE_SAMPLES ... native=... requested=... effective=... max=16`
- `RR_GI33_JITTER ...`
- `RR_GI33_QUALITY_RESET ...`
- `RR_GI29_DLF_MODE ... requested=1 enabled=1`
