M12R86 - native primary material color with PT lighting

Primary PT now traces diffuse transport without the approximate base texture color.
The original reviewed raster material shader supplies its actual blended/detail
color at the existing diffuse-plus-specular combine. PT diffuse is multiplied by
that color; PT specular stays separate. Primary emission, fog, coverage and alpha
remain in the native shader. No native lighting is added to covered PT pixels.
Black channels need no albedo division. Secondary-hit materials are unchanged.

The final PT packet is now 64 rather than 32 bytes per pixel, carrying separate
lobes through fresh RR and standalone reconstruction. This adds about 253 MiB per
retained 4K final buffer (113 MiB at 1440p). Existing memory caps remain; this is a
material correctness change, not an FPS optimization or a complete lighting fix.

Known unresolved: RR diffuse guides still approximate layered material albedo;
secondary materials still use the simplified decoder; reflection hit-distance
and motion guidance, emissive sampling, missing sampler bindings and environment
coverage are not repaired by this change. R85 early reservations are retained.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Run Collect-PT-Primary-M12R86.cmd after normal gameplay.
