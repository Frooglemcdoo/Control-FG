# Full PT conversion — development checkpoint, not a user build

User requirement: all lights evaluated through PT, no baked or native lighting.
R102 was visually worse and its authored no-shadow policy is rejected.

Completed in this checkpoint:
- Every selected analytic light uses traced visibility again.
- Production shader no longer imports hit/miss/final-bounce baked GI.
- Removed post-trace GI compute dispatch and its PSO/sidecar dependency.
- Reflection-probe environment lighting is disabled; an unlit scene miss is black.
- DXIL validation rejects all GI request/cache and environment-probe bindings.

Not complete and not advertised as full PT:
- Emissive extraction and area-light MIS are integrated, but runtime scene
  coverage and performance have not yet been verified in Control.
- Unsupported primary materials and correspondence failures retain native
  composition; transparent/specialized material models are not replaced.
- Scene completeness still depends on the captured native TLAS and light lists.
- The four-hit limit remains. There is no physical sky-source import.

Shipping package is deliberately disabled while these gaps remain. This avoids
asking the user to judge a darker intermediate state as a purported lighting fix.

Next implementation: replace specialized primary/fallback shading with PT material handling and
verify that production composition cannot source native illumination when PT owns
a pixel. Ownership/synchronization failure must remain explicitly diagnosed.

Reference: https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer
and https://pbr-book.org/4ed/Light_Sources/Area_Lights .


## Retained emitter integration

The same prepared scene/SBT lease now supplies per-instance triangle extraction.
A compute-only builder filters emissive materials, records world-space triangles,
UVs and alpha-test policy, and retains its resource through recording and GPU
completion. Submission admission gates geometry reads. Its extended native root
uses 58 DWORDs; all native compute bindings are restored before DXR.

The path producer samples this registry at every scattering vertex, evaluates
emission textures and cutouts, traces visibility, and combines direct sampling
with BSDF-hit emission using matching selection/solid-angle PDFs and the actual
primary lobe proposal. Last-bounce direct lighting has no nonexistent competing
continuation. Incomplete/overflow registries disable explicit sampling and its
MIS weights together. No reflection-probe or diffuse-cache radiance is imported.
The registry is bounded to 65,536 triangles and reports count/invalid/overflow
through PT_PRIMARY_EMITTER_GPU. Native TLAS exclusions remain exclusions.

Target shadow endpoints use a conservative triangle bound from the existing
NVIDIA spawn implementation and the inverse instance transform, including
nonuniform scale, shear and reflection. A one-ULP distance exclusion alone was
insufficient to establish that the emitter cannot shadow itself.

Checks include CPU triangle/PDF/MIS integration and scene mapping, WARP production
extraction (filtering, bounds, transforms, overflow) and production integrator
readback (direct+hit MIS, shadows, missing registry, last bounce, unsupported
secondary emission). Production DXIL and extended native compute root/PSO checks
are required. These tests do not establish in-game image parity or all-material
PT ownership. Packaging remains disabled; this is not an installable full-PT fix.

## Opaque material composition conversion

The six captured character pixel programs now have independent, hash-locked
composition profiles. Material color is captured at the original diffuse/color
combine, but PT is selected only after the final character light-rig addition.
Selecting at the earlier combine would retain native character lights. One
variant packs surface RGB in XYW; the adapter explicitly remaps that output.
All original bytecode remains private. Only identities, offsets and binding
requirements are retained. Existing nine general profiles are unchanged.

Observed opaque modes 0, 1 and 3 now use the PT diffuse/GGX closure, including
materials with native scattering-kernel indices. This deliberately converts
those native models; it is not a recreation of native skin, cloth, or subsurface
scattering. Mode 2 and unknown modes are still rejected. Primary normals, F0 and
roughness come from the GBuffer, with original raster material color retained.
No primary emission is added by these character adapters. Emission handling for
existing general profiles remains after their lighting selection.

GPU regression fixtures inject a distinct native light-rig contribution after
the material combine. Covered pixels must exclude it, and uncovered/rejected
consumers must preserve it, for both XYZ and XYW layouts. Local private capture
checks cover all fifteen profiles, checksum rebuilding and rejection behavior.
The production integrator fixture verifies converted modes and unknown-mode
rejection separately. Native fog/media, glass, decal lighting, complete scene
light import and runtime image validation remain outstanding.
