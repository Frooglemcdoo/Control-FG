# Primary light adapter

Source is the captured primary lighting shader
`a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f`.
`pt_native_primary_lights.hlsli` implements light selection, incident radiance and
view-space direction/target. It does not supply unoccluded light as final surface
radiance: the path adapter must transform to world space, trace visibility from
its safe spawn point, and evaluate the BSDF with the returned selection PDF.

| Input | Primary binding | Evidence |
| --- | --- | --- |
| Light constants | b2 | RDEF `deferredlight_constants`, 416 bytes |
| Point/spot/sun totals | b2 bytes 256/260/264 | RDEF fields and DWORD 514 sun-count load |
| Point records | t10 | 160-byte structured elements |
| Spot records | t15 | 320-byte structured elements |
| Sun records | t18 | 576-byte structured elements |
| Spot projection | t14, s11 space1 | DWORD 7561 SampleLevel |
| Sun projection | t17, s8 space1 | DWORD 660 SampleLevel |
| Material Part1 / Part3 | t7 / t8 | RDEF; not t3 from the ray-generation pass |

Do not use the deferred-reflection ABI here: that pass uses b1 for its light
constants and different spot/material/projection slots. Primary b1 is the
16-byte `mid_translucency` cbuffer. The scene-hit library now permits a
`PT_SCENE_MATERIAL_REGISTER=t8` specialization; its default remains t3 for the
existing fixtures and ray-pass layout.

Point attenuation follows DWORDs 11333–11527: smooth 20% range fade, reciprocal
pi times squared distance plus squared radius (floor 0.0001), subtract falloff.y,
clamp nonnegative, and optional squared cone factor for flag 4. Spot attenuation
follows DWORDs 5752–5918 and 7323–7574: projected frustum bounds, range fade using
unnormalized clip Z, wedge-distance/sin-cone/radius denominator (floor 0.25), and
projection atlas with optional flag-1024 fisheye mapping. Sun follows DWORDs
526–696: stored direction, RGB, projection XY without W division, and
`1 + transparency * (sample.x - 1)`. Native shadow-map terms are deliberately not
reused as visibility for another path vertex.

Selection is uniform over all point and spot lights plus the reviewed single
sun. Out-of-range/frustum lights remain zero-contribution samples; resampling
would change the PDF. Invalid strides/counts, nonfinite data and multiple suns
are rejected. These are delta-light estimators with native radius attenuation;
they do not yet implement finite-area emitter sampling or radius-based soft
shadows. The integrator now passes its per-path RNG state to the light adapter.

Validation: both default-t3 and primary-t8 scene/path libraries compile with DXC,
as does the light sampler. The Windows fixture executes the same light shader
on WARP with separately calculated point/cone, spot projection, sun projection
and selection-PDF expectations. It also exercises out-of-range, singular,
nonfinite and count-overrun cases. This proves shader execution against those
fixtures, not live native descriptor capture or lighting/material parity in game.
The native primary pass/root bindings and traced-visibility caller still need to
be connected before the renderer can use this adapter.
