# R87: per-light spatial reconstruction

R86 traced full-resolution four-sample shadows without reconstruction. The user
reported 19 FPS, hard edges and multiple silhouettes. R87 replaces that path:

- One spatially distributed emitter sample on a half-resolution grid (one anchor
  per 2x2 quad), independently per light. Full-resolution native BRDF energy remains.
- Reuse only a valid same-light anchor on the same receiving surface. Geometry or
  light-list boundaries receive four direct samples instead of borrowing blindly.
- Reconstruct each light's visibility from nearby samples before applying that
  pixel's diffuse/specular contribution. Depth-plane and normal tests reject
  incompatible surfaces; tight contacts keep their direct result.
- Overlapping tiles provide filter borders, but only non-overlapping interiors
  write the game outputs. No frame history, CPU waits or per-frame allocations.
- Stable spatial sampling avoids a frame-random seed. This is spatial filtering,
  not NRD or a temporal denoiser; residual spatial noise or blur must be assessed.

Multiple real lights may still cast distinct shadows. This build does not merge
light identities or remove lights to hide their silhouettes. The intended change
is reconstructed soft edges rather than four coherent emitter-sample silhouettes.

`rt_all_gpu` now reports reconstructed (untraced) task counts separately. Evidence
comes from the center tile including its border; valid Lighting counts cover only
its interior. Ray counts apply only to the inspected task subset, not a full frame.

Close Control to install, copy all files beside Control_DX12.exe, keep native RT
Contact Shadows enabled, and compare F10 NATIVE SHADOWS / ALL RT SPOTLIGHTS live.
Check FPS, Jesse's shadow, object silhouettes and soft edges; then SAVE LOG.
In-game performance and appearance still require confirmation.

Reference for filtering inputs and area-shadow reconstruction:
https://developer.nvidia.com/blog/rtx-coffee-break-ray-traced-light-area-shadows-and-denoising-712-minutes/
