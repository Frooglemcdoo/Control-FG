# GI sampling replacement: evidence and accounting checkpoint

## Research

Read the original Control implementation in [Ray Tracing Gems II, chapter 46](https://developer.download.nvidia.com/ray-tracing-gems/rtg2-chapter46-preprint.pdf), sections 46.4 and 46.6.1 (printed pages 749–750 and 756 onward). Its near-field GI uses cosine-distributed rays and substitutes precomputed lighting for misses. The published design excludes secondary specular highlights to reduce variance. Its native denoiser includes mip-luminance firefly clamping, temporal accumulation and spatial passes. Bypassing that pipeline removes its firefly protection; this does not establish that restoring it is compatible with RR or solves ghosting.

The chapter is design evidence, not a guarantee of the installed binary's exact behavior. Our GI6 disassembly contains direction-dependent shading terms. Their presence alone does not establish an active specular lobe or prove they cause the artifact. Earlier secondary-Fresnel suppression did not fix it. Trace the active data flow before changing these terms; do not use the chapter as grounds to delete them.

NVIDIA's [ReSTIR GI integration guide](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/RestirGI.md) instructs implementers to verify sample creation and shading without reuse before adding resampling. It requires actual proposal PDF and secondary geometry/radiance, and describes the direction-independent radiance approximation. See also its [shader API](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/ShaderAPI-RestirGI.md) for target-density, Jacobian and visibility integration. Revision pinned on 2026-11-01.

## Implemented checkpoint

`tools/gi_candidate_contract.py` is an offline local-selection accounting reference, not RTXDI integration. It enumerates all outcomes and their probabilities, preserving native throughput, the total native ray denominator, depth scale and miss fallback. It explicitly rejects evaluation at another pixel. Native weighted contributions must not be divided by a directional PDF again.

The R277 capture round trip covers 3,600 pixels and 21,600 rays, including 363 all-miss pixels. Expected RGB differs from captured native final GI by at most 1.4771e-8. Deliberately dropping miss fallback produces maximum absolute error 0.00951; renormalizing hits by hit count produces 0.65193. These are native linear-GI values, not display-space errors. The checked-in JSON records the exact trace hash and results.

Seven unit tests cover mixed RGB contributions, all-miss and black-hit cases, missing rays, invalid/nonfinite inputs and forbidden cross-pixel evaluation.

Reproduce:

```sh
python tools/test-gi-candidate-contract.py
python tools/audit-gi-candidate-roundtrip.py /path/to/GI-Ray-R277-57620.bin --output /tmp/candidate-roundtrip.json
```

## Next implementation boundary

This local selection cannot find bright paths absent from the original six rays. It is not a noise-reduction result. Before runtime spatial reuse, establish the producer's actual proposal density and finite-ray support; separate reusable secondary radiance from source-dependent response; evaluate the receiving pixel's response and geometric Jacobian; and perform fresh connection visibility. Preserve the miss estimator and depth transition. A geometry-only neighbor gate cannot replace visibility and would risk leaks through walls or foliage.

The next code milestone is a no-reuse runtime candidate path that reproduces native GI, followed by a spatial-only experiment with valid reconnection. Temporal reuse should wait until this works, so it does not obscure the existing ghosting question. If the native directional terms cannot be reevaluated reliably, do not move their total radiance across pixels; evaluate guided ray sampling instead, with its own correct PDF and support.

No game shader or runtime setting changes in this checkpoint. No new user capture is needed to repeat these checks. Neither the capture nor this reference proves the specific plant/hair speckles fixed, and no performance gain is claimed.
