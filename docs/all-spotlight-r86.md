# R86: reduce live spotlight visibility cost

R85's log confirms valid lighting for all 32768 sampled receivers and valid
visibility/contributions for all 4096 inspected tasks. The sampled batch contains
about 112000 light/pixel pairs. At 32 samples per pair, that single batch requested
about 3.6 million rays. Frames were approximately 7.5 seconds apart.

R86 changes:

- Reserve compact work once per active GPU wave using prefix sums, instead of
  every receiver spinning on the same atomic counter. Capacity remains bounded;
  near exhaustion the original individual reservation policy is retained.

- Extract and validate native per-light diffuse/specular energy before DXR.
  Exact-zero pairs are valid zero contributions and skip tracing entirely.
  Nonzero energy is never threshold-culled; invalid extraction still retains native lighting.
- Stop traversal at the first accepted shadow blocker. Native material alpha tests
  remain active, so transparent cutouts may still reject an intersection.
- Use four deterministic area-light samples per active pair instead of 32.
  This reduces the requested ray count eightfold before zero-energy culling.
  Soft penumbrae have fewer samples and may look more stepped; this build does not
  add a temporal denoiser or claim that quality/performance is already verified.
- Keep full-resolution receivers and all native tile lights. No arbitrary light
  truncation or distance cutoff is introduced.

`rt_all_gpu` now includes `sampled_rays` and `zero_contributions` for the inspected
subset (up to 4096 tasks). These are not whole-frame timings or ray totals.
`nearestBlocker` now means the closest returned accepted blocker among samples,
not a nearest-surface query; composition consumes binary visibility only.

Close Control to install beside Control_DX12.exe. Keep native RT Contact Shadows
enabled. F10 switches NATIVE SHADOWS / ALL RT SPOTLIGHTS live, without restarting.
Test the affected room, then SAVE LOG (Lighting-R86-*.log). Check FPS and the soft
shadow edges. Existing RR, glass, menu and RTX 40-series fixes are retained.
