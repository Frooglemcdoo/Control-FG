# Terminal diffuse GI-volume input

R86 capture shows zero environment scale, missing static illumination, weak floor
reflections and remaining binding fallbacks. The native deferred reflected-hit
shader 4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9
reads volume transforms, a sparse ternary tree and six directional irradiance
bricks. The PT shader read none of them. The final native GI output is not a
baked-only source: restoring it also restores native ray-traced lighting.

R87 ports the diffuse volume subset at native DWORD1377..2735. Authored volume
transforms use world position; the sampling basis multiplies the stored world
normal. The six coefficients use squared positive/negative directional weights.
Overlapping volumes use native fade/priority normalization, additive volume bias
and native deferredlight_constants[15].w / pi. No brightness constant is invented.
The native diagnostic-orange sentinel is skipped. SH, environment fallback,
direct lights, shadow atlases and native reflected/RT-GI result textures are absent.

The lookup runs only at the fourth surface vertex after direct lighting/emission,
when the existing integrator would stop. It adds kd * cached diffuse irradiance/pi
through the current path throughput; the same primary-lobe split classifies it.
Earlier vertices do not receive the cache. This is a biased terminal diffuse
approximation for the omitted indirect tail, not an unbiased complete PT solution.
It does not supply terminal glossy transport, baked-only direct lights, emitter
NEE, full secondary materials or complete RR guides. It may therefore only partly
improve the user's missing-light/reflection report. No visual fix is claimed yet.

Same-view environment capture clones b4 and t16..24 into private b4 and t21..29.
Native b3 illuminationvolume and the exact scale are copied into private b5 via
an owned upload allocation. Constants are rechecked at admission, descriptor
versions are covered by the existing submission guard, and the existing same-view
native environment submission must precede PT. As with existing native light
SRVs, data lifetimes/states rely on that native queue dependency. A descriptor
copy alone is not ownership of its resource. Cache capture is optional: missing
inputs disable only this tail. Constant mutation after capture cancels the frame
under the existing input-validity policy, not a silent stale-resource read.

Bounds cover volume count (384 and buffer size), structured strides, top-level
index products, child pointers, 32 tree steps, brick address arithmetic and finite
radiance. Outside volumes contributes zero. No new path rays or dispatches.
Upload allocation grows 1792 to 2048 bytes per retained producer; no new per-pixel
storage. GPU diagnostics use previously unused fault-counter slot15, byte96, for
positive terminal cache samples on the existing 64x64 sparse grid.

Validation: actual HLSL on Windows WARP with directional coefficients, oblique
normals, basis rotation, overlap normalization, native scale/bias, parent/leaf/
internal tree paths, absent inputs and malformed bounds; full existing build and
regression gates. WARP validates implementation, not in-game appearance or speed.
