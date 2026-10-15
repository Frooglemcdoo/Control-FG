M12R70 - reconstructed PT surface replacement, root-buffer composition

This re-enables replacement with structural changes rather than reverting PT:
- No per-draw cloned descriptor tables and no 64/1024-table budget. Surface
  shaders read a retained raw GPU buffer through an appended root SRV.
  Coordinates are checked before every raw load. Native roots/arguments are
  restored after each draw; descriptor heaps and material tables stay native.
- Raw PT samples go through a separate reconstruction pass. Spatial neighbors
  must agree in material, normal and surface plane. History is reprojected and
  rejected on material/normal/depth disagreement, frame discontinuity, changed
  resolution, or a large camera jump. Covered black remains PT; unsupported
  primary pixels remain native. Smooth specular uses a smaller history budget.
- Primary ownership no longer depends on a stochastic secondary BRDF hit. An
  unsupported secondary terminates the estimate at its accumulated finite
  radiance. This is incomplete secondary-material coverage and can lose energy;
  it is not a claim that special materials have acquired a correct PT model.
- Reconstruction restores both native compute and graphics bindings after its
  private heap is used. History/raw outputs remain retained through GPU fences.

Close Control, extract beside dxgi.dll, and replace package files. Existing PT
startup selection and game settings remain unchanged. Do not rerun PT-Enable
if already enabled. Collector: Collect-PT-Primary-M12R70.cmd.

Windows tests cover raw-buffer commit, 200 patched shader draws, bounds/native
fallback, root restoration, reconstruction noise reduction, history acceptance
and rejection, resolution/camera discontinuities, and admission/replay safety.
These are component tests; in-game visual quality and performance need review.

Remaining limits: native passes still execute for unsupported consumers. This
is not the completed FPS redesign or all-material/all-light implementation.
Reprojection currently uses camera/depth data, not native object-motion vectors;
animated surfaces can reject history or retain trails. Native delta lights are
still delta lights, not physically sized area emitters. The new buffers use a
bounded 2 GiB pool and add reconstruction work; no FPS improvement is promised.
