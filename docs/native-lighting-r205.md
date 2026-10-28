# R205: restore native reflected-light view direction

R204 projection rays were visually accepted in the supplied 2026-10-28 capture.
Its manifest reports native_projection_blend, MAP GUARD OFF, RR ON, SPEC Native,
hit distance OFF, effects 0x6B, and a complete capture with no failures.

R205 removes CONTROL_SPEC_HIT_VIEW from the experimental proxy build. Native
secondary-hit point/spot specular shading again uses the hit-to-camera view
vector instead of the R184 hit-to-primary-surface vector. The scheduled native
shader remains active, preserving the R172 load-lifetime optimization. Startup
logs identify this route with SPEC_NATIVE_VIEW build=R205.

This restores the requested pre-R184 behavior; whether it resolves the reported
missing angled highlights still needs an in-game comparison. It does not enable
a previously disabled reflection component: the full reflection signal was
already active. RR noise may differ when highlights move or brighten.

R204 ray selection, authored clipping, filtering, map guard, and R193 long
reflection sequence remain unchanged. Startup still uses emitter rays with
MAP GUARD ON. For the accepted lighting setup choose F10 RAYS: PROJECTION and
MAP GUARD OFF, leave RR ON and SPEC Native, then close F10 and let it settle.
Compare the reflective surface at the same grazing angle where its light
highlight went missing. Send a screenshot and F10 SAVE LOG if still absent.

The F9 component-isolation suite remains diagnostic; judge this change during
normal gameplay with no suite running.
