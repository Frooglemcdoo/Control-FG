# R83 exact primary-angle guide

R82 capture: 26 logged guide submissions accepted, 23 RR evaluations succeeded,
no reconstruction setup failures, consumer rejections or late-expired producers.
Later sparse coverage: 1807..2004 / 2040 sample positions. These are spatial,
periodically logged samples, not temporal pixel transition counts. They cannot
locate or prove visible native/PT switching. Primary material/depth/normal
rejection remains possible and must not be bypassed to conceal artifacts.

The existing RR shader used abs(normalView.z) for the EnvBRDF horizontal
coordinate. That is the forward-axis approximation, not per-pixel perspective
N.V. The PT integrator already computes the exact local outgoing direction from
the validated shading normal and primary ray; its z is the needed cosine.

R83 transports that value through raw packet byte 60 (previously zero/unused).
No buffer format stride or lifetime changes. PTMaterialGuide validates admission,
raw/final coverage, material identity, diffuse coefficient, and cosine range.
The guide retains native F0 before replacing diffuse albedo; specular becomes
F0 * EnvBRDF(actual_NdotV, smoothness).x + EnvBRDF(...).y. Unsupported/native
pixels retain their prior guide convention. RR-off PT shading remains identical.

NVIDIA reference: specular reflectance depends on NoV, and reflections in motion
need specular motion vectors or primary-surface specular hit distance + matrices:
https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md
Sections 4.1.8, 4.1.9 and 4.2.1. R83 does not implement hit distance. The current
mixed-BSDF continuation distance must not be mislabeled as a specular guide.

Validation uses the real optimized guide compute on D3D11 WARP with an angular
LUT, distinct .25/.75/1 cosines, independently computed expected reflectances,
invalid-angle/native fallback, zero diffuse reflectance, and guard cycles.
Existing shared-integrator WARP fixture checks emitted primary cosine. Neither
fixture establishes in-game visual completion or renderer performance.

Correction to the preceding research response: the fault counters already
separate primary rejection (PT_PRIMARY_FAULTS) from retained-primary secondary
termination (PT_PRIMARY_SECONDARY_TERMINATION). R83 adds an explicit stage label;
no extra counters, filtering, weakened gates or altered admission are introduced.

Remaining work: stable primary correspondence with matching final color/guide
selection; correct reflection-motion/distance producer; texture footprint/mip
sampling review (PT currently requests mip zero); true light-source coverage.
Environment remains zero in the capture. Missing lights are not solved here.
