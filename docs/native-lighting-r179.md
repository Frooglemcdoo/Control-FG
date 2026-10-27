# R179: distinct rejection retries for SPEC 2–4

The goal is the stationary bright-reflection material stability of SPEC 16 (or at least 8) at a 2–4 ray budget. SPEC 8's remaining noise is established; this is not another unchanged sample-count comparison. R176 is the base. Rejected R177/R178 experiments are absent.

## What changes

The native variant-0 ray generator already importance-samples a truncated GGX lobe. If a direction fails its surface-facing test, the retry sequence depends on the pixel and frame but not the outer sample index. All rejected samples at that pixel therefore use the same replacement direction. R176 caches this identical calculation without changing the result. At low ray budgets this can leave multiple rays tracing the same direction.

R179 gives rejected samples separate deterministic pseudo-random retries when the effective ray count is exactly 2 or 4. Seeds include the native world-cell hash, both native noise-texel values, temporal/sample index and retry index. Including noise texels is important: pixels sharing a quantized world cell must not receive identical fallback seeds. Uniforms use the upper 24 hash bits scaled into [0,1). The hash is integer-only until that conversion.

Primary directions are unchanged. The native lobe mapping, 0.4 truncation, surface-facing acceptance threshold, 100-attempt cap, ray count, ray distance, hit/miss handlers and serial shading remain. At other counts the original fallback arithmetic and R176 memoization remain. The new shader adds conditional control flow, so identical high-count sampling is not a guarantee of identical GPU timing.

There are no extra rays, allocations, history buffers or dispatches. Rejected low-count samples do more direction-generation arithmetic than R176 because each now finds its own valid direction; performance still needs measurement. The existing 60% clamp, skin/hair guides, GI and centered spotlight rendering are unchanged. Gating is by the shader's effective count, not the menu label or RR state.

## Evidence and limits

`test-spec-low-sample-retries.py` executes the emitted integer hash instructions and checks them against an independent implementation. It verifies source scope, unchanged trace calls/hit handlers, retry cap and distinct inputs. The Windows build assembles and validates the actual DXIL and runs the existing state-object substitution/failure tests. Baseline checks protect 132 renderer/guide files.

The numerical test uses the native angular mapping in a normal-aligned frame and a synthetic bright angular lobe. It measures 2/4-sample variance and mean against independently sampled valid directions. This omits scene visibility, textures, RR reconstruction and GPU costs. It is not evidence of in-game SPEC 16 quality. Native correlated retries and decorrelated retries can have different mean estimates; no radiance is deliberately clamped or scaled, but reflected brightness must be checked in game.

This is a limited first sampling change. It cannot improve pixels whose primary directions never fail the facing test. An alternative short stratified sequence was investigated offline and not selected because several synthetic cases regressed. Light-aware sampling and reconstruction remain further work if this change is insufficient.

## Test this build

Close Control and install the package beside Control_DX12.exe, preserving the existing NVIDIA runtimes. Restart with NATIVE BLEND, RR on, FG off, GI 6 and SERIAL shading. SPEC resets to Native on startup: explicitly choose **SPEC 4**.

Inspect the established problem material with a direct bright reflection after the camera stops. Check noise, reflected-light brightness and detail, then use PERF CAPTURE. Try **SPEC 2** afterward. Judge against the already established R176 quality reference; no repeat SPEC 8 characterization is needed. Include the Lighting-R179 log and performance ZIP. `spec_retry_create` should report `created=1 low_sample_retries=2,4`; creation failure falls back to native and cannot validate this experiment.

Experimental branch `work/r179-spec-low-sample-retries` only. No production branch, release or deployment is changed. The repository is public; this branch is not private.
