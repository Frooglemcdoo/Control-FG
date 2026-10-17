# M12R101 native light radius

R100 capture: no binding failures; last HitGI 1771 requests, 1768 positive,
zero unavailable. The cached diffuse correction remains active.

Native evidence: validation/primary-variants/single-spot.dxbc, SHA256
`a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f`.
Decoded point instructions DWORD12090..12533 and spot DWORD7644..7983
implement finite-radius lobe broadening. Radius comes from point byte144
and spot byte72. Existing adapter used radius for attenuation only.

The shared helper reproduces the native acos polynomial and projected circle
fraction above the horizon. Area = min(fraction * radius^2 / (2 distance^2), 1).
Above native threshold 0.000333333359, b=1/(1+3 area), variance=(1-b^2)/(6b-2b^3),
and effective roughness=sqrt(material roughness^2+variance), clamped .05..1.
Zero radius/distance retains roughness before that direct-light clamp.

Both reservoir target and final direct BRDF use this effective roughness.
Original material, indirect proposal, guides and geometric visibility remain.
No added rays, image buffers, payload or sampled area emitters. Shadows still
use the center target. This addresses highlight width, not missing geometry or
all reflection coverage. Diffuse radiance and native attenuation are unchanged.

Validation covers 2500 double-reference combinations, limits, horizon behavior,
invalid data, off-axis highlights, unchanged diffuse, RIS/final consistency and
shadows. WARP fixtures check radius/distance ABI and independent GGX results
for point, spot and directional lights. Existing shader and integration gates
remain required. In-game result is not asserted by these numerical tests.
