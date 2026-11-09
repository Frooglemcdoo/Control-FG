# R71: correct full-distance spotlight filter inputs

R70 was installed and active in the user's log. Its four-sample change did not
resolve flicker, pop-in or dithering. This build addresses concrete problems in
the inherited short contact-shadow path; visual improvement is not yet verified.

- Spotlight rays now use RAY_FLAG_NONE, allowing closest-hit traversal. R70 used
  ACCEPT_FIRST_HIT_AND_END_SEARCH: its returned distance could be an arbitrary
  traversal-order blocker. Taking the minimum across four such distances did
  not produce a reliable nearest blocker for the penumbra filter.
- The disk emitter uses twice the stored emitter radius as its diameter, matching
  the radius used by the native spatial filter. The previous short-contact rule
  randomly multiplied that diameter by 1–8. Sampling still varies across the disk
  and keeps four samples and soft edges, but the physical disk size stays fixed.
- Both native spatial passes get a spotlight-only radius/distance bound of 0.99.
  The inherited sqrt(1-ratio) approximation becomes invalid above 1 and its
  tangent becomes singular at 1. Full-distance blockers can reach that domain.
  Below 0.99 the computation is unchanged; this bound is an approximation for
  blockers close to/inside the emitter, not a complete area-light solution.

Native point shadows, geometry inclusion, alpha handling, light selection and
composition are unchanged. R67 RR filtering and R69 nonblocking logging remain.
The two new spatial shaders must load and create successfully before the mode
becomes available. Only the verified full-distance pass selects them.

This does not repair missing TLAS casters or previous-light history identity.
Those remain possible causes of residual coverage changes. Closest-hit traversal
can cost more GPU time than early-out; FPS must be checked on the user's GPU.

Install ALL files with the game closed, including rt_spot_spatial0.dxbc and
rt_spot_spatial1.dxbc. Compare NATIVE and RT SPOTLIGHTS live with F10 in the same
stairwell/hallway, stationary and moving, RR on. Save one Lighting-R71 log.

Validation: signed DXIL validator; captured-IR checks preserve point/alpha/hit
handlers; emitted filter bounds and history tests; Windows CI disassembly/WARP
PSO validation and DLL build. These do not substitute for game rendering tests.

Reference: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_ray_flags
