Control primary PT - M12R31

Close Control and replace all package files. F10 opens the overlay; F11 unchanged.
Test Jesse in the same bright room, previously missing object lights, and moving
through the area. Run Collect-PT-Primary-M12R31.cmd and send the ZIP.

R31 preserves the game's native direct diffuse/specular lighting and material
processing. R30 replaced those with a simplified geometric-normal BSDF and one
random direct-light sample. R31 only adds diffuse-first indirect PT transport;
first-hit direct/emission and specular continuation are excluded. Later hits
retain diffuse/GGX transport. Native diffuse is never zeroed. The exact bound
specular UAV format must have proven typed-load support; otherwise PT retains
native rendering and logs native_specular_typed_load_unproven.

Materials with native translucency-depth ranges are unsupported by this opaque
integrator and retain their native pixels, alongside special BRDFs. This keeps
scattering materials out of the simplified shading path.

This remains a hybrid indirect-lighting development build. It is not a full
standalone PT replacement or a completed replacement of native diffuse GI.
Over-bright indirect lighting, denoising, and remaining secondary-material
accuracy still require visual validation; no in-game success is claimed.

R30 logs show typical prepare+validation around 12-16 ms, with a logged spike
of 25.0+21.6 ms, while PT GPU work remains around 4-6 ms. R31 avoids repeating
identical fence-order checks for each allocation. The cache is separate for
preview and submission and is invalidated with every graph-clock revision.
No ordering, ownership, native fallback, or admission checks are removed.

CPU, Windows/WARP, shader and packaging checks do not establish in-game FPS.
