# R96: retain R94 selection, test close shadow coverage

The latest user test reports that R94 resolves the earlier shadow problems,
with close-up shadow disappearance remaining. The attached R94 log records
unshadowed-contribution selection. A previous report of a system green screen
is still unresolved; this package does not claim to diagnose or fix that crash.
R95 was a recovery rollback prepared before this clarification, not the baseline
for this experiment. R96 restores R94 selection and preserves its shader bytes.

## Change and hypothesis

R94 rays used one shared maximum distance: distance to the native cone position
minus fNearDistance. Their directions target samples around a different native
light position and were computed from the unshifted surface, while rays start
at the native biased origin. Those different measurements can exclude blockers.
The new selector admits candidates previously rejected by shadow maps, making
this existing coverage limit worth testing again. This is not proven to explain
the user's missing close shadows. R73 tried full segments before the selection
fix without success; R96 tests them with the successful R94 selection and
retains R94 radial sampling and mean blocker distance.

RT CLOSE COVERAGE measures each sample from the actual biased ray origin to
that sample's emitter position, normalizes that vector with a zero-length guard,
and stops one part in 2^20 before its endpoint. The original fNearDistance no
longer shortens these segments. Each hit is compared to its own maximum. Only
occluded samples enter the mean blocker distance, preserving R94 softness.
Ray count, emitter radius, random sample positions, native caster inclusion,
alpha hit handlers, point branch, selection, composition, and filters stay fixed.
This still replaces one selected spotlight per pixel, not every shadow.

Two ray generation exports and two aligned records are prepared at state-object
creation. The live mode picks an existing record; it does not allocate, read back,
wait for the GPU, or add dispatches. The record buffer grows from 64 to 128 bytes.

## Live comparison

Close Control to install all files beside Control_DX12.exe. Preserve existing
NVIDIA runtimes. Enable native RT Contact Shadows. F10 offers:

- RT R94 BASELINE: exact R94 selection, rays, composition and filters.
- RT CLOSE COVERAGE: same selection and filtering, corrected ray segments.
- RT RAW and the diagnostic signal views use the corrected segments.

Compare baseline and close coverage at the same camera, then near/far from the
light. No restart is needed between modes. Keep RR settings the same between
comparisons; turn RR and FG off if interpreting signal views, whose final image
still includes downstream material/lighting/postprocessing. SAVE LOG records
Lighting-R96. Do not attempt to reproduce the earlier system crash.

## Validation and limits

DXIL validation; generated-instruction execution for zero length, endpoint
geometry, near-light blockers and unequal hit/miss extents; preserved native
handlers and point branch; packaged hashes; Windows WARP/runtime checks.
These do not establish real-GPU stability or in-game visual correctness.
Tracing closer to fixtures may reveal fixture occlusion that the cutoff hid.

DXR defines the ray as Origin + T * Direction with intersections restricted to
the supplied extent: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
