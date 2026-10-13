Control-FG M12R3 - INTEGRATED PRIMARY PT TEST

This build connects the private scene/SBT, four-hit path tracing and supported
opaque material composition. It is an experimental rendering build, not another
CPU-only scene capture. Windows compilation and WARP checks do not establish
in-game coverage, stability, performance or image quality on your RTX 5090.

INSTALL
1. Close Control. Back up your current dxgi.dll and matching pt_*.dxil / pt_*.dxbc
   files together, so you can restore the approved P2/G3 build as one set.
2. Extract this package beside Control_DX12.exe and replace its included files.
   Keep the rest of your existing working mod/Streamline installation.
3. Start Control DX12. Keep your existing DLSS/RR settings and native RT enabled.
   This requires the previously verified Steam native renderer/D3D DLLs.

ONE COMBINED TEST
1. Load the same entrance save. Leave F11 in normal mode (do not cycle it).
2. F10 opens the overlay. Use the PT Reflections toggle inside the overlay
   to compare OFF / ON from the same camera; close the overlay for each view.
   Let each view run about 20 seconds, then move through the entrance briefly.
3. Note visible changes, noise/flicker, artifacts, frame rate and any crash.
   Screenshots of the same view with PT OFF and ON are useful.
4. Close Control, then run Collect-PT-Primary-M12R3.cmd and upload its results ZIP.
   There is no three-minute capture requirement and no M6/M11 collector to run.

WHAT IS ENABLED
- Four surface hits, one path per pixel; Lambert/GGX transport, native point/spot/
  sun sampling with traced visibility, secondary emission and environment misses.
- Nine reviewed general opaque surface shader variants. Unsupported materials,
  unmatched primary visibility and rejected frames use their native surface term.
- Native alpha, fog/media and primary emission remain in their original shaders.
  Approved P2/G3 continue to serve their existing reflection/glass paths.
- The PT Reflections overlay toggle controls the added PT result. F11 modes disable the
  primary replacement so they continue to show the existing diagnostic paths.

LIMITS FOR THIS FIRST INTEGRATED BUILD
- Geometric normals and a new diffuse/GGX material model; normal-map and special
  material parity are not established. Character/decal/SSS coverage is not claimed.
- Delta lights, not area-light soft shadows. No new PT-specific temporal denoiser
  or accumulation is included. Existing RR remains in the installed mod's path.
- Native upstream lighting still runs for compatibility. Extra tracing, geometry
  copies and queue ordering can reduce frame rate. No performance gain is claimed.
- Admission may fall back if a native writer/view/binding is unavailable at the
  required boundary. The collector distinguishes CPU submission from GPU-covered
  samples. A successful DLL load alone is not reported as successful PT rendering.

ROLLBACK
Close Control and restore the backed-up dxgi.dll and shader files together.

M12R3 CHANGES
- Support native COMMON geometry buffers using D3D12 implicit read promotion.
  Other states still require validation and explicit state restoration.
- Track only command lists containing observed AS work or scene tickets in the
  scene journal. Ordinary graphics/copy list resets no longer consume its slots.
- Skip primary draw/dispatch admission immediately if the catalog is invalid.
- Read the complete camera from authenticated current-thread native providers;
  match each pass against only the constants that it actually consumes.
- Retain an immutable private camera upload for tracing; restore the native CBV
  afterward and verify original pass-specific bound bytes at submission.
- Log first catalog failure and the geometry-copy rejection stage.

KNOWN STATUS
R2 produced 75-85 FPS OFF and 40-46 ON on the user's RTX 5090, with no primary
PT dispatch. Its log showed 3050 failed geometry copies, an invalid catalog and
rejected camera transforms. R3 repairs concrete mismatches exposed by those
results. Windows/WARP verification is not in-game primary coverage confirmation.
The P2 reflection path still requires its verified two-layer resources; the R2
run had one layer and fell back to native. R3 does not weaken that resource guard.
