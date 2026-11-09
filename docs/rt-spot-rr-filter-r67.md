# R67: RR filtering for replacement spotlight shadows

R66 shadows are visually promising, but the user reports strong stationary noise with RR.
The RR contact-shadow hook neutralized temporal and spatial filtering even when R66
replaced short contact rays with full-distance single-ray visibility. R67 preserves
native filter arguments only inside a pass that actually recorded replacement rays.
Requested-but-untraced modes and native fallback retain the existing RR policy.

The R66 ray, composition and history-reset shaders are unchanged. This does not
increase sample count or force filtering settings that the game has disabled.
The existing transition reset still applies when switching native/replacement shadows.
RR skin, guide generation, reflections and broad diffuse filtering are unchanged.

## Test

Install with the game closed. Enable native RT Contact Shadows, enter the same room,
and select RT SPOTLIGHTS in F10. Compare RR off/on while moving, then stand still
for 10 seconds each. No restart between comparisons. SAVE LOG writes Lighting-R67.
The log records rt_spot_filter with RR state and preserved temporal/spatial arguments
on first use and changes. CPU recording is not proof of GPU output or visual quality.

## Validation

Policy matrix covers RR/contact flags, replacement state and native filter arguments.
The Windows scope test covers requested but untraced, native fallback, actual full
rays and scope exit. Exact baseline exceptions protect unrelated renderer code.
Visual improvement still requires the user's in-game comparison.
