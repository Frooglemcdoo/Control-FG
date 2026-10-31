# R257 plant/cabinet RR input audit

User capture: 2026-10-31, PID 38204. All nine archive hashes and every restored chunk hash verified by `tools/package-lighting-capture.py`. 67 restored files. Four captured frames: Clamp Only / Smooth On 53726–53727; Both 58100–58101. Both has six native spatial passes and temporal history; Clamp Only has zero spatial passes and the R217 current-frame smoother. Both pairs have successful RR output, reset=0, current-frame filter observations and matching GI resource identities. Input 2560x1440, output 3840x2160, preset F (6), hit distance unbound, pre-exposure 5.55555534, exposure scale 1.

## Confirmed guide coverage gap

In normalized plant ROI x=.49–.62, y=.49–.76, 31,231–31,273 leaf-material pixels per frame have invalid replay RGB (the intentional -1 uncovered sentinel). All receive constant diffuse-guide RGB 0.054901123 instead of textured foliage reflectance. The material table index is 1307 in the first three frames and 1306 in the last; these are observations, never stable identifiers for a fix. The hole persists in Both: changing filtering hides symptoms without repairing this guide.

The shader explicitly falls back to `materialGuide` when replay RGB is invalid (`src/shaders/rr_live_guides.hlsl`). This value derives from packed material F0, not textured diffuse reflectance. Standard cabinet/wall surfaces in the inspected crop have textured coverage, so the foliage gap does not explain every affected surface.

`src/rr_albedo_shader.h` rejects differing native/replay vertex bytecode. Existing foliage evidence in `validation/foliage-variant` contains a real mismatch involving vertex deformation; removing that guard is not a justified fix. The current log reports zero admitted foliage at each sampled preparation event, consistent with missing replay coverage, but does not contain a same-frame per-draw rejection audit. Do not assert the exact rejection reason for this plant from older evidence alone.

## Lighting and geometry observations

The captured plant-region normal guide agrees with independent native GBuffer decoding within 0.000489 per component; roughness agrees within 0.000487. This supports the packing/decoding convention in these pixels, not complete correctness of all geometry/motion/exposure inputs.

In a flat foreground-planter ROI x=.52–.57, y=.76–.85, raw-GI luminance high-pass RMS is 0.000139–0.000143 for Clamp Only versus 0.0000558–0.0000589 for Both (Gaussian sigma 1 input pixel). Diffuse accumulation contains the same variation. Specular input also contains stochastic variation in both modes. Output high-pass RMS includes real surface texture and is not an artifact-only metric. Camera/jitter differ between captures; unaligned temporal differences must not be reported as pure noise or causal proof. Four stationary frames cannot certify ghosting behavior.

## Corrective scope

1. Recover the affected foliage draw's native material/vertex/pixel contracts and exact rejection reason. Implement textured albedo coverage preserving native geometry, alpha coverage, bindings and depth behavior; do not bypass vertex safety or hard-code material indices.
2. Audit diffuse/GI composition and guide consistency on covered cabinet surfaces separately. A larger history/spatial filter is not yet an evidenced integration correction.
3. Keep the accepted R257 package/storage and R217 no-history behavior until an isolated correction has bytecode/pixel/runtime validation. No rendering changes are made by this audit branch; it is not a fix build.

Reproduce with Python, NumPy and SciPy:

`python tools/analyze-rr-plant-guides.py <restored-capture> --output pixel-audit.json`

Metrics and source capture metadata are evidence; the raw game captures are not checked into this repository.
