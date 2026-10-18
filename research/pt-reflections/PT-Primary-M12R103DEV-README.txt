M12R103DEV - experimental PT test update

For the SAME Control DX12 installation used for the previous PT builds.
This is an update: keep the existing Streamline/NVIDIA files and settings.
Close Control, extract every file beside the existing dxgi.dll, and replace
the matching files. Do not mix individual DLL/shader files between builds.
No renderer.ini or user settings are supplied or overwritten by this ZIP.
Keep the existing PT selection; PT-Enable.cmd is optional if it is disabled.

Changes since M12R102:
- PT opaque transport no longer reads baked GI or reflection-probe lighting.
- All sampled analytic lights have traced visibility again.
- Emissive scene triangles are sampled as area lights, including reflections.
- Six character and two projected decal shader paths now compose PT lighting.
- Grazing-angle normal handling and reconstruction/RR guides agree.
- Unused native environment/probe data cannot cancel valid PT frames.
- Includes pt_emitter_build.dxil, required by the new emitter extraction pass.

NOT a completed full-PT release. Native glass lighting, fog/volumetrics and
fallback for unsupported/rejected surfaces remain. Complete scene/light import
is not proven. True environment misses are black; no physical sky is imported.
These remaining paths do not yet satisfy the requested removal of all native
lighting. Windows compilation and WARP checks are not in-game visual proof.

Test the same lobby/reception/floor camera positions used for R101/R102.
Leave resolution, RR, FG and exposure settings unchanged for the comparison.
Check reflected fixtures, floor light pools, texture flicker and frame rate.
After testing, run Collect-PT-Primary-M12R103DEV.cmd and return its results ZIP.
It retains emitter counts/overflow, sidecar load failures, PT coverage and
fallback diagnostics. Add screenshots at the same positions if possible.

Build.json identifies the exact source commit; SHA256SUMS.txt covers package
files. Master/main have not been changed. This is an experimental branch build.
