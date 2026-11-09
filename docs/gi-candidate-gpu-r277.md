# No-reuse GPU accounting prototype

`src/shaders/gi_candidate_no_reuse.hlsl` implements the native weighted sum and its surface/fallback split as a standalone compute shader. It does not sample neighbors, use history, change light energy, or run inside the game yet. It is not a ReSTIR reservoir implementation. Source-pixel throughput cannot be transported to a neighbor without reevaluation.

The shader consumes bounded per-pixel ray arrays and preserves the native total-ray denominator and operation order. Invalid ray counts, ray ordering, material sentinels, negative/nonfinite lighting, and overflow fail closed. The harness validates native final RGB, separate hit and miss terms, counts, and untouched dispatch guard memory under the D3D12 debug layer.

## Fixture provenance

`research/gi-source-r267/candidate-gpu-fixture.json` contains seven complete pixels extracted from trace SHA256 `5d7992f6fa5ec78066314f35bc647b0565a5c42eb4e360083b5d39d8d1fd9efb`: the four brightest by maximum RGB, then distinct all-miss, all-hit, and mixed pixels in descending brightness order. Each retains all six rays and its recorded native final RGB. Two synthetic cases exercise known colored-energy and black-hit denominator outcomes. The harness adds seven invalid-input cases.

The Python fixture builder emits exact float32 bit patterns. These selected cases supplement the existing 21,600-ray offline check; they are not full-frame GPU validation or evidence of improved visual quality.

## Research refinement: directional terms are not proof of a specular lobe

The pinned native GI6 disassembly has this data flow (SHEX DWORD offsets):

| Range | Native operation | Consequence for reuse |
| --- | --- | --- |
| 1059–1079 | Normalize secondary minus primary position into r14 | Direction depends on source pixel |
| 2532–2589 | Dot secondary normal with negative r14, sample EnvBRDF LUT and scale r4 | Total shaded radiance is not automatically direction-independent |
| 2741–2767 | Normalize negative secondary view-space position into r11 | Analytic-light response uses a camera-dependent vector |
| 4846–4982 | Compute spotlight half-vector, Fresnel complement, N dot L and 1/pi before accumulation | This is a diffuse attenuation factor; the half-vector alone does not prove an added specular highlight |

This is compatible with the published design's intent to omit noisy secondary specular highlights while retaining diffuse energy corrections. The old no-secondary-Fresnel test did not fix the speckles. Do not remove these terms based on their names or substitute the camera vector with a guessed primary vector as an unmeasured fix.

The next integration needs either explicit evaluation of those terms at the destination or a proven separation of reusable and source-dependent components, plus connection visibility and correct proposal weighting. The existing trace has no evidence that such reconnection is already valid.

## Validation and scope

The dedicated `gi-candidate-validation.yml` workflow compiles the shader with the Windows shader compiler and executes it through D3D12 WARP. WARP exercises the API and shader contract, not RTX performance. DXC also compiles the source locally with warnings treated as errors.

This branch does not change the shipped DLL, settings, Native Blend, DirectStorage, or capture controls. No user capture is required for this test. A game build would still need resource lifetime, transitions, descriptor binding and output-equivalence checks before enabling reuse.
