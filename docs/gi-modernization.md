# Modern RT indirect GI replacement

Goal: replace the indirect-GI estimator incrementally, retaining Control's scene,
materials, authored lights and RR output contract. Higher resolution is a later
quality control; it does not by itself solve isolated high-energy ray samples.

## Implemented foundation

`src/gi_reuse/initial_sample.h` is shared C++/HLSL code for the initial, no-reuse
sample adapter. It separates secondary radiance, primary BRDF times cosine and
source solid-angle PDF. The source PDF is divided exactly once. It preserves
miss fallback separately and normalizes by all configured rays, including black
and miss rays. It rejects stale frames, missing verification, invalid sentinels,
nonfinite input and overflow. `gi_initial_sample.hlsl` supplies a bounded GPU
entry point. This is **not yet RTXDI integration or a game-bound renderer**.

The existing R277 native-throughput accounting prototype remains the reference.
Do not reconstruct a physical PDF by arbitrarily splitting captured throughput
into a BRDF and PDF: that only proves algebra, not the producer distribution.
The new adapter deliberately requires producer-established PDF and conventions.
No runtime hook selects this shader; no native lighting changes are shipped.

## Existing R285 evidence

All eleven supplied archive parts passed their SHA256/chunk/range checks.
Eight complete source frames: 4,281,103 records, 115,200 sampled pixels,
691,200 rays, no overflow. The first trace and first full-resolution RR capture
share engine frame 4002. All eight streams pass the native lighting accounting
checks (maximum absolute output accounting error 3.725290298461914e-8).

In the inspected hair ROI, frame 2 pixel (405,361) has luminance 0.2006007303;
spotlight 8 supplies 0.1848002059 (92.12%). In the plant ROI, frame 8 pixel
(1539,975) has luminance 0.1474954507; spotlight 8 supplies 0.1367471378
(92.71%). A different plant sample, frame 2 pixel (1413,921), is 98.63%
point light 1. IDs are session-local and do not name a lamp or permanent material.
First-frame pre-filter GI RGB and GI-at-RR-input RGB are identical. Alpha differs.
These observations locate high-energy samples before RR, not a proven geometry
or PDF defect. The shifted spatial sampling grids are not same-pixel temporal
histories.

## Integration gates and sequence

1. Own the producer proposal: record or generate the actual direction, unshifted
   hit point, geometric and shading normals, sampling density, and finite-ray
   support. Existing stored hit positions subtract 0.01 from hit distance;
   stored interpolated normals cannot stand in for geometric normals.
2. Bind the no-reuse adapter and reproduce the intended native estimator. Keep
   miss-volume fallback and depth scaling; never normalize only by hits.
3. Integrate a pinned NVIDIA RTXDI ReSTIR GI version. Supply primary material
   evaluation, proposal/target PDFs, geometric Jacobians and native alpha-aware
   visibility. Native secondary shading can be direction-dependent; do not
   silently label it reusable Lambertian radiance.
4. Validate spatial reuse first, then temporal reprojection/disocclusion. Hair,
   foliage, moving objects and shadow boundaries are acceptance cases. No
   history accumulation is enabled merely because records are numerically valid.
5. Feed compatible signals/guides to RR; measure cost before raising resolution,
   rays or bounce count. Compare against native and a high-sample reference.

Source: NVIDIA RTXDI ReSTIR GI integration documentation, accessed 2026-11-02:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RtxdiApplicationBridge.md

## Validation

Portable test: `g++ -std=c++17 -Wall -Wextra -Werror tools/test-gi-initial-sample.cpp`.
Windows workflow `gi-modernization.yml` compiles the shared CPU code and shader,
executes the shader on D3D12 WARP, compares CPU/GPU results, checks invalid inputs
and nonmultiple dispatch bounds, and checks the D3D12 debug layer.
No in-game quality or performance improvement is claimed by these tests.
