# Native GI producer support audit

The audit now disassembles both hash-pinned GI ray libraries and verifies the
following data flow. Results are in `research/gi-source-r267/sampling-audit.json`.

* The secondary normal store is fed by three packed signed vertex normals,
  barycentric interpolation and the WorldToObject normal transform. It is a
  shading normal, not a triangle-edge-derived geometric face normal. Smooth
  surfaces can have different shading and geometric normals.
* The secondary position store uses `O + D * (RayTCurrent - 0.01)`, followed by
  the view transform. The actual hit point is available earlier in closest hit
  from O, D and unmodified RayTCurrent, but is not what R278 exported.
* Each ray's TMax is the base distance in DiffuseGIConstants[0].x multiplied by
  approximately `0.8 + 0.4*w`, where w is the third fractional sequence coordinate.
  It is not a fixed per-dispatch range. Both producer variants use this rule.
* TraceRay uses flags 0, instance mask 1, hit offset 0, geometry stride 2 and miss
  index 0 in the audited libraries. This does not alone validate another shader
  table or prove BLAS geometry flags/material behavior at runtime.

## Consequence for reuse

The native finite-radius estimator mixes hit shading with miss fallback. Moving
a sample to a neighbor can change whether a randomized ray reaches that surface,
in addition to changing its direction and geometric Jacobian. A fixed-distance
gate and cos(theta)/pi alone do not establish the correct estimator.

For illustration only, if w were independent uniform noise, the probability of
reaching distance d would be 1 below 0.8*base, decrease linearly through the
transition, and be 0 above 1.2*base. Independence and that marginal distribution
have not been established for the game's correlated sequence and texture input.
Do not install this illustrative probability as a measured native PDF.

The connection prototype now additionally requires verification that randomized
finite-ray support is represented in its estimator. The old geometry/PDF-only
flags are rejected. This is a developer contract, not automatic verification.
No native range, sample pattern, light intensity or displayed lighting changed.

The next producer adapter needs the unshifted hit point, true triangle geometry,
the actual generated direction/range and a defensible proposal/support model.
Destination-dependent GI6 shading and native alpha-aware visibility remain
required before reuse. These findings do not prove that the jitter or interpolated
normal is a native rendering bug or the cause of the plant/hair speckles.

## Checks

Run `python tools/test-gi-producer-support.py --dxc /path/to/dxc` to check both
native libraries plus eight deliberately changed range, hit-bias, normal and ray
flag cases. The existing connection tests reject the old support flags. Shared
HLSL still compiles with warnings treated as errors. No new game capture is needed
to establish these static producer facts.
