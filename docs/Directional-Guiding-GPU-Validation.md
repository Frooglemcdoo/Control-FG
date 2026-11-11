# Directional guiding GPU validation — 2026-11-11

Research commit: 07a01d21d3cfee793c5098d1f5f563fc370b66c7.
[Windows validation run](https://github.com/Frooglemcdoo/Control-FG/actions/runs/43021017680), job 130523928523: success.

## What passed

- C++: 1 million directional samples and 7.5 million world-space mixture samples; weighted energy, support, HDR normalization, rotated/flipped receivers and endpoint cases.
- Windows SDK 10.0.26100.0 DXC compiled Sample, Learn, Prepare and Publish as cs_6_0 with warnings treated as errors.
- D3D12 WARP executed 640 proposals across five receiver orientations, including 40 explicit endpoint cases, with CPU direction/PDF/contribution parity.
- Learning leaves the current frame's frozen proposal unchanged. Publication and consumption of the next guide execute without a CPU readback between frames. 401 next-frame PDFs changed.
- Stale frame/generation rejection, empty-guide fallback and output guards passed.
- Prepared 8-row/8-column CDF removes per-ray 64-cell normalization. This is an operation-count improvement, not a measured FPS result.

## Corrections made during validation

HLSL 2018 loop variable scope warnings were fixed. Exact-zero cosine coordinates could produce zero density and are now mapped to a half-bin interior coordinate. World-sphere poles are kept inside the representable boundary to preserve azimuth. Density is evaluated from the generated direction to handle finite-precision cell boundaries. CPU random coordinates use explicit 24-bit conversion.

The initial Windows CPU assertion did not record its failing inputs. The exact-zero defect was independently reproduced; it is not established as the original assertion's sole cause.

## Limits and integration requirements

This is a standalone compute fixture using synthetic fresh shading. WARP is a software D3D12 renderer, not RTX hardware. No Control ray-generation or lighting-evaluator binding, receiver tracking, game image improvement or FPS gain has been demonstrated. The serial reference learning kernel is not suitable for production scale.

The current game package remains R423. These research files do not change its rendering.

Before enabling this in-game:
1. Feed actual fresh per-ray lighting and its exact generation/PDF into scalable learning.
2. Establish receiver correspondence without using transient raw material IDs as persistent keys.
3. Ensure ray generation and evaluation share the same frozen guide and per-ray correction, including miss/volume contributions and zero terms.
4. Preserve native finite-range sampling and hit-distance contracts; validate descriptors, shader-table records and fenced resource lifetime.
5. Test moving receivers, disocclusion and changing lights against the same native ray budget, then measure RTX cost.

The previous learning trial still applies: a stale proposal can increase variance. World orientation and correct weighting do not by themselves solve changing illumination. No new capture is requested by this validation.
