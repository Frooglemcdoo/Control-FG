# R203: paired emitter and native-projection shadow rays

R202 found 279 samples for the affected spotlight with lit native-map centers
and blocked raw RT. Offline reconstruction confirmed native projection and
emitter directions differ by 6.88–42.68 degrees for those samples. R203 measures
whether changing the path actually clears the blockers, without changing the
image. This is diagnostic, not a lighting fix or a restoration of R162.

## Capture

Use Native Blend, MAP GUARD OFF, RR ON, SPEC Native, hit distance OFF and the
accepted game RT settings (including Ray Traced Contact Shadows). Stand outside
the affected window, F10 -> TRACE SHADOW, and hold still until completion.
Send the Shadow-R203 folder. One capture is sufficient; no free-camera trip or
ON/OFF retest is required. The selected settings remain unchanged.

## Measurement

Two extra capture-only rays run at each valid sample. Both retain the native
biased receiver, TLAS, frame, mask 1, alpha handler and translated hit table.
One targets the emitter; the other targets an apex solved from the actual native
projection matrix. Each is independently clipped against the authored near
plane. Both carry full hit identities, direction, endpoint limit, pixel, light
and frame. Existing full reference and geometry-only miss probes remain intact.

The capture reads original t4 spotlight data and b0 view rows without replacing
their descriptors. Two additional private output banks share the existing
submission/fence/readback lifecycle. Unsupported matrices/segments emit two
zero records, explicitly classified as unsupported rather than misses. The
CPU decoder independently reconstructs both paths from captured composition
light records and ray context and rejects mismatched identities or geometry.

Emitter hit plus projection miss supports a path-dependent blocker discrepancy;
both hitting keeps caster/bias differences open. Neither result alone validates
a rendering fix, proves the exact screenshot seam, or identifies persistent
assets. Native receiver bias and shadow-map rasterization still differ from RT.

## Validation

Local DXC compile; decoder tests for geometry, independent clipping, wrong
identity, swapped/missing/duplicate/partial pairs, and unsupported projections;
classifier checks and unchanged clean-baseline validation. Windows CI executes
the shared projection helper on WARP with known apex/clipping fixtures and
runs the integrated export-retry/resource guards and normal build checks.
Actual DXR hits in this room still require in-game validation.
