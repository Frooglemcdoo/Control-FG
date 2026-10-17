# R100: complete secondary diffuse tails at authored hit locations

## Evidence
R99 capture PT-Primary-M12R99-Results-20261017-084758-753.zip matches commit
0ef1b98e81d892012555c672bd845ac9bccbfbe2 with no collection warnings. No binding,
consumer, handoff or admission failures; no secondary termination records.
Last frame2338 has 36 miss requests/36 positive cache samples, total GI positive314,
primary faults25 BRDF/5 normal. Lights9 point/28 spot/environment0. These sparse
samples do not prove visual completeness. User reports improvement but missing
reflections and lighting inside them.

Remedy/NVIDIA Ray Tracing Gems II chapter46, sections46.1.2 and46.2.4, describes
precomputed directional irradiance and sampling it at reflected hit locations
for the next diffuse bounce. It also documents camera-frustum TLAS limitations.
https://www.researchgate.net/publication/354065089_Ray_Tracing_in_Control
NVIDIA RTXGI sample shows discrete diffuse/specular selection with reciprocal
branch probability applied to throughput:
https://github.com/NVIDIA-RTX/RTXGI/blob/main/Samples/Pathtracer/Pathtracer.hlsl

The R99 integrator queried GI at a bounded miss or fourth surface only. Diffuse
chains could attenuate authored light repeatedly or end elsewhere before using
it at the reflected surface. R100 replaces the secondary diffuse indirect tail
at a covered hit; this is an explicit irradiance-cache approximation, not a claim
of unbiased equivalence to a full scene path tracer.

## Estimator and safety
After direct/emission, at depth>0, eligible materials choose specular with p from
PTSpecularProbability or diffuse cache with 1-p. Diffuse emits throughput*kd/(1-p)
for E/pi at this hit and terminates. Specular samples a SPECULAR-ONLY BSDF and
weights by 1/p exactly once. Retaining the full mixture BSDF here would double
count diffuse energy. Primary sampling and first-lobe output fractions remain.
Outside authored volume coverage or with admission disabled, normal multibounce
transport is unchanged. Final-hit and miss handling remain. Zero cache is valid.
A malformed brick lookup returns zero and logs cache_unavailable; it is never
replaced by invented ambient light. Coverage checks bounds, active sentinel,
finite transforms, fade/priority and positive weight before selecting the cache.

Shared volume-weight code runs unchanged in the GI compute resolver, while raygen
only checks volume membership. No brick traversal/sampling or callable is added to
DXR. Existing t21/b4/b5 descriptors were already retained and copied for GI. The
new request tag3 uses the existing two scratch buffers. Metrics add three counters
and padding to preserve 8-byte timestamp alignment (DataBytes1208).

## Validation
Production-integrator WARP fixtures extend to14 cases, covering diffuse cache at
hit, coverage disabled, mixed-lobe probability, specular continuation, pure glossy
transport, direct/emission once and all prior miss/terminal guards. GI compute
fixtures extend to15 cases including hit-cache validity, outside and black cache.
GI sampler fixtures extend to14 cases and also validate the shared membership
predicate, including malformed transforms, zero fade and empty volume sets.
Production shader compile, native-root and resource-lifetime gates remain required.
Visual correction still requires the user's in-game comparison.
