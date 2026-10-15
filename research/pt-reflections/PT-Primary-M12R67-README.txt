M12R67 - PT surface-lighting integration build

This build activates the separately validated full-surface PT producer. On the
nine reviewed general opaque material variants, PT direct lighting, diffuse
bounces and specular bounces replace the combined native surface lighting.
The output uses the internal render extent selected by DLSS, not display size.
Primary emission, fog/media, alpha and depth remain in the native compositor.
Black PT samples are valid replacements, not a reason to show native lighting.

This is NOT the completed all-material/all-light or FPS redesign. Native passes
still execute for characters, scattering, transparency, unreviewed shaders and
failed path/admission fallback. Their GPU cost has not yet been removed. The
roughly 2.7ms scene-preparation path and broad AS lifetime wait are unchanged.
No FPS gain is promised. Point/spot/sun sampling uses the current native light
buffers; finite-area soft shadows and emissive-light importance sampling remain
unfinished. The new surface result has not been visually validated in Control.

PT-off sessions retain the compute-only hook configuration. Startup PT selection
and all game graphics settings are preserved. No renderer.ini is included.

Install: close Control and extract all package files beside your existing
dxgi.dll. Keep the DLL and shader sidecars together. Launch with PT enabled;
if it is already enabled, do not run PT-Enable.cmd. PT remains restart-only.

First check the same lit room and reflective surfaces in Quality. Check Jesse,
bright fixtures and glass for regressions. Native fallback on special materials
is expected. RR may reduce noise, but denoiser/guide compatibility is not yet
visually proven. A short visual check is the purpose of this integration build,
not another DLSS scaling sweep. Close the game and run
Collect-PT-Primary-M12R67.cmd if reporting a problem; it records both PT producer
and surface consumer evidence without requiring a large new capture.

Rollback: close Control and restore the complete R66 package. Its DLL ignores
the extra pt_primary_surface.dxil file. Saved settings remain unchanged.
