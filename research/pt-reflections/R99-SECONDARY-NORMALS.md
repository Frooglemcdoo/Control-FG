# R99: preserve interpolated mesh normals at secondary hits

R98 capture: sampler replay reaches count2400 with no binding failures or
consumer rejections; native GI remains active. Missing reflections therefore
cannot be attributed solely to the previously rejected sampler bindings.

Code evidence: PTSceneClosestHit interpolates packed vertex normals and applies
the inverse-transpose world transform into PTSceneHit.shadingNormal. Before R99,
PTDecodeSurface always initialized secondary surface.normal to geometricNormal
and never used shadingNormal. This flattens reflected-hit lighting and subsequent
reflection directions on smooth meshes. It does not explain all flat-floor gaps.

R99 selects the already normalized mesh normal, oriented to the two-sided
geometric normal. Invalid/non-unit/tangent normals and normals excluding the
viewer fall back to geometry; the primary G-buffer normal admission is unchanged.
No normal-map approximation is introduced. The geometric spawn offset is retained.
PT_PATH_SURFACE_GEOMETRY enables geometric hemisphere checks for direct lighting
and every continuation, including primary surfaces. RIS targets exclude candidates
below geometry; the integrator also rejects their direct contribution. Rejected
continuations terminate without resampling, preserving the sampling PDF.

Reference: Physically Based Rendering, BSDF representation:
https://www.pbr-book.org/4ed/Reflection_Models/BSDF_Representation
The geometric frame determines reflection/transmission while the shading frame
evaluates the BSDF. This is camera radiance transport, not bidirectional importance
transport. The fallback is a robustness policy, not exact native material parity.

Validation: shared HLSL/C++ transport tests cover interpolation, face orientation,
non-finite/degenerate/non-unit normals and silhouette fallback. Production-integrator
WARP cases extend the existing 16 to 20: tilted direct lighting, geometric rejection,
cosine response, reversed mesh orientation. Existing shader, GI and binding gates
remain required. Actual game visual validation is pending.

No extra GPU buffers, DXR payload words or rays. RR optional hit-distance and motion
are unchanged; this build does not claim their absence is the proven floor cause.
