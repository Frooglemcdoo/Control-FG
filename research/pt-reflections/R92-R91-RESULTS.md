# R91 user result and R92 corrections

User reports floor reflections and baked/static lighting remain missing.
Reviewed local attachment PT-Primary-M12R91-Results-20261016-150929-660.zip and
sampled frames from 20261016-2007-13.7866465.mp4. R91 log has 55 throttled camera
resolution records, all accepted=0/reasons=8, with last counter28200. No successful
pending-camera resolution was logged. Counts are not frame-wide coverage rates.
Last sparse GPU sample: frame3750, 2040 sampled, 2027 material matches, 2025 depth
matches, 1980 covered, 45 primary rejections. Terminal GI positive samples285.
These measurements show execution, not restored visible lighting.

Decoded all nine original surface shaders with decode_dxbc.py. The reviewed
3755ae5f... and 68b095104... PS variants read inverse extent and viewpoint.y but
NOT matrix registers6..9. The captured matching VS a954d6e56122c5c0008775c37371d1bf240c6349c89c3397c455df655cdc1ea1
reads all those matrix xyz lanes. R92 gates that exact VS identity, requires
root8, merges only its matrix into the pixel extent for matching, then separately
authenticates pixel extent and vertex matrix at submission. This is foliage
coverage, not a demonstrated cause of missing floor lighting.

Native reflected-hit shader4fc0a0e9... material metadata defines emission intensity
at byte48, emission RGB multiplier at100, emission texture at material*35+5.
Instructions7130..7195 form emission independently of scattering. The custom
secondary decoder returned on unsupported scattering before reading any emission.
R92 preserves finite front-face emission at such hits, with the existing path
throughput exactly once, and still ends the unsupported path. No BRDF substitution,
extra brightness, forced primary ownership, or fabricated light source is added.

Remaining major gaps: analytic local sources treated as delta lights despite
native finite-radius fields; authored GI only evaluated at terminal vertex4;
emissive NEE absent; complete secondary BRDF/material support absent. Neither
R91 results nor these source corrections establish visual resolution of those
issues. Do not report a clean build as an in-game lighting fix.
