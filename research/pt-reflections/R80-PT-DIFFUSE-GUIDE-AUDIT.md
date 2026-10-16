# R80 PT diffuse guide correction

The R79 capture `probe-20261016-142823-016-7356.log` has zero reconstruction setup failures, two consumer rejections with producer_recorded=0, and sampled successful RR evaluations with fresh PT input. Custom temporal-history counters are zero on the fresh route as intended. Optional specular distance remains unbound. Sampled video frames show residual mottling and motion smear; neither exact event alignment nor a quantitative visual improvement is asserted.

The actual RR shader `rr_live_guides.hlsl` used DecodeMaterialValue as diffuse albedo. For admitted PT BRDF mode zero, this value comes from packed specular tint and GBuffer2.g. `pt_primary_path.hlsl` instead uses the material texture and multiplier, times (1-F0), for the diffuse transport coefficient. These are different quantities. This explains an integration mismatch, not proven causality for every remaining artifact.

NVIDIA's primary programming guide describes diffuse albedo as the diffuse reflectance component at input resolution, in linear space:
https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md#411-diffuse-albedo
Accessed 2026-10-16.

R80 uses raw PT kd only where current validated surface coverage and material ID agree with the RR GBuffer. The diffuse coefficient is no longer clamped in the raw packet; the standalone filter applies its unchanged 0.02 denominator floor when reading it. Normal/roughness and specular guides remain unchanged. Invalid or unsupported samples preserve native guide values. No reflection-distance data is misrepresented as specular distance.

The bridge borrows no unowned GPU address: RR slots retain the PT frame through existing slot/fence retirement. It requires exact engine frame, render extent, producer submission, armed admission, matching queue and fresh-input mode. The PT mutex is acquired with try-lock because native guide locks are already held; contention returns a reason instead of blocking. This intentionally declines unsubmitted producers rather than assuming cross-list ordering. Diagnostics distinguish binding availability from per-pixel coverage; a bound source does not mean every pixel is PT-covered.

Tests execute the real guide shader on WARP across off/on/off modes and compare output data, including exact preservation of native normals/specular and invalid/uncovered diffuse pixels. Black is a valid diffuse material, not a missing-data sentinel. Shader reflection verifies the added raw-buffer bindings. Existing reconstruction tests cover moving the denominator floor to read time. Windows compilation and in-game capture remain separate gates.
