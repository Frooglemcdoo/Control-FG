# R180: capture the stationary RR highlight breakup

R179 loaded successfully (`created=1`, no substitution failures) and reached effective SPEC 4, but the user reports no fix. The supplied painting and elevator screenshots show coarse high-contrast highlight grain at SPEC 4, remaining grain at 8, and substantially smoother highlights at 16. Motion is not the reported problem. The target remains SPEC 2–4 with quality approaching 16, or at least 8. No further sample-count characterization is requested.

This build uses R176 rendering, excluding R179's retry experiment and the rejected R177/R178 changes. It changes capture access and validation, not the reflection shader, RR guides, denoising settings, clamp, shadows or skin/hair behavior. It is a diagnostic, not a visual fix.

## Why a new capture build

In the existing diagnostic build, `CONTROL_CAPTURE_DIAGNOSTIC` removes the RR-before-evaluation capture call, selects native SR capture instead, and the overlay rejects RR-on capture modes. The collector also requires `native_sr` with RR inactive. That path cannot answer why RR reconstructs the bright reflections poorly.

The new `CONTROL_RR_MATERIAL_CAPTURE` build flag enables the existing RR-before and RR-after capture calls and excludes the native-only capture route. CAPTURE RR admits NATIVE BLEND with RR runtime work enabled. PACKAGE RR passes `-RequireRR` to the collector. The collector now requires successful active RR output, reset=0, all three exact RGBA16F material guides at input resolution, native G-buffers/depth/motion and accumulated lighting. It rejects RR-off captures rather than mislabeling them as RR evidence. The older native collector mode remains available without the new switch.

Existing five-second settling, consecutive-frame pairing, device/state validation, resource lifetime checks, fenced background export and two-pair session limit remain. Allocation and readback occur only after a capture request. Do not interpret FPS during capture as ordinary rendering performance.

## What is captured

Each position produces two consecutive evaluated frames with:

- Actual pre-RR color and post-RR output, plus depth, motion, jitter, exposure/reset metadata.
- Accumulated `lighting-specular` and `lighting-diffuse`, separate from the material-guide files called `specular` and `diffuse`.
- RR normal/roughness, specular-albedo and diffuse-albedo guides actually supplied to the evaluation.
- Native GBuffer1/2, raw GI, replay albedo and hair mask when bound.

Accumulated lighting is not an isolated direct-light contribution or necessarily the earliest raw reflection signal. Input/output resolutions differ, so comparison requires registered/common-resolution views; screenshot subtraction alone is not a causal test. Two frames can locate grain and detect input variation but cannot fully characterize convergence over time.

## Source audit so far

The live roughness guide uses `1 - GBuffer1.B`, matching the native reflection shader's smoothness inversion. Roughness is packed in normal alpha and the native NGX packed-mode parameter is set and read back. This does not establish correctness of every affected pixel. NVIDIA documents linear roughness and normalized shading normals for RR: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md .

Specular reflectance uses the existing material F0 and native EnvBRDF LUT. The ordinary path's view-angle approximation and the R106 perspective comparison are already documented; R180 does not repeat that experiment. R102 already established textured diffuse guides in the earlier painting regions. R103's native filtering and R111's temporal experiments also already exist; none is silently reenabled here. The missing evidence is the actual settled input/guide/output relationship at these current problem surfaces.

The supplied screenshots alone do not prove that RR interprets the noise as texture. That remains a hypothesis. The capture will distinguish grain already prominent in accumulated specular lighting from grain predominantly preserved or emphasized by reconstruction, and permit checking the exact material guides against the native G-buffer.

## Capture instructions

1. Close Control, install R180 beside Control_DX12.exe, preserve the existing NVIDIA runtimes, and restart.
2. Select NATIVE BLEND, RR on, FG off, GI 6 and SPEC 4. Keep those settings for both positions.
3. Frame the problem painting. Open F10, click CAPTURE RR, then hold still until Saved pairs: 1/2. The panel closes automatically; remain in gameplay.
4. Frame the elevator highlight. Click CAPTURE RR and hold still until Saved pairs: 2/2.
5. Click PACKAGE RR and send every ZIP in the opened SEND folder. Include a screenshot identifying each view. No PIX or Nsight session is needed.

If capture reports failure, PACKAGE RR collects the diagnostics. Do not switch RR off to make capture work. No SPEC 8/16 comparison is needed for this test.

Experimental branch `work/r180-rr-material-capture` only; no production, release or deployment changes. The repository itself is public.
