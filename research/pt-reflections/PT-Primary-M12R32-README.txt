Control primary PT - M12R32

Close Control and replace all package files. F10 opens the overlay; F11 unchanged.
Test the same room and camera position with RT and PT, then move and zoom.
Run Collect-PT-Primary-M12R32.cmd and send the ZIP with a matched screenshot pair.

The primary diffuse PT bounce now uses the current native GBuffer shading normal,
including smooth shading and normal-map detail, instead of the triangle normal.
Ray spawning keeps the geometric offset. Rays below the geometric surface
terminate without resampling; invalid/back-facing guides retain native lighting.
Secondary materials still use the simplified geometric-normal material model.

R31 logs: median preparation 8.07 ms, late validation 3.81 ms, PT GPU 4.13 ms.
R32 uses dense row mapping instead of per-row tree allocations, removes
duplicate graph selection within the locked scene snapshot, and
reuses successful CPU preflight when the entire scene input is identical and
no allocation/geometry mutation has occurred. Any mutation uses the original
full preflight and table comparison. Late scene ordering is always rechecked.
Validation logs now report submission_preflight_reused or rebuilt so the next
capture can establish whether the fast path helps in the actual game.

Native direct light and reflections remain intact. This still adds PT diffuse
indirect lighting to native output; native indirect overlap, denoising, and
secondary material parity are unfinished. This is a hybrid development build,
not a finished full PT renderer. The normal fix does not resolve all lighting
coverage issues. No in-game FPS or visual success is claimed from build tests.

Shading-normal reference (camera/radiance transport; no importance correction):
https://www.pbr-book.org/3ed-2018/Light_Transport_III_Bidirectional_Methods/The_Path-Space_Measurement_Equation
