# GI25 — RR Temporal Input Parity

This build abandons the native-denoiser hybrid path and returns to pure Ray Reconstruction.

GI25 adds two missing DLSS-RR temporal inputs while leaving Control's native denoisers bypassed whenever RR is active:

- A full-frame `DLSSD.ResponsivityMask` in `R16_FLOAT`, exposed as a signed -100..+100 slider on the RR settings page. The default is -50. Zero disables the optional mask. The mask is bound only for RR Model F.
- `FrameTimeDeltaInMsec` is written and read back on every RR evaluation using the measured interval between RR evaluations, clamped to 1..100 ms.

Negative responsivity bias is intended for the stability/noise test; positive values favor faster temporal response. Changing the bias forces an RR history reset.

Native GI/spatial/temporal denoising remains bypassed exactly as in the public v2.1.1 RR path. No GI21/GI22 prefilter and no GI24 hybrid behavior is present.

Expected log proof:
- `RR_GI25_RESPONSIVITY_READY`
- `RR_GI25_RESPONSIVITY ... bias=...`
- `RR_GI25_TEMPORAL_INPUTS ... frame_time_ms=... responsivity_bias=... responsivity_bound=1`
- `RR_NATIVE_EVALUATED ... responsivity_bias=... frame_time_ms=...`
