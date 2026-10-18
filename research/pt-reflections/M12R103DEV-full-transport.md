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
- Unknown primary modes and correspondence failures retain native composition;
  transparent/decal lighting and native volumetrics are not replaced. Reviewed
  opaque modes use the PT closure, rather than native specialized models.
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

## Primary shading-horizon continuity

Full-surface PT now regularizes valid primary raster normals toward the geometric
normal when their shading hemisphere excludes the camera. The blend enforces a
small positive geometric/view margin and varies continuously through grazing
angles; ordinary normals are unchanged. All scattering evaluation and sampling
uses this same frame. Geometric visibility and robust spawn offsets are retained.
Non-finite/non-unit inputs and geometrically back-facing views still reject.
This addresses an actual rejection category in R101 logs without accepting bad
resource correspondence or switching to native light at the shading horizon.

The shared CPU/HLSL tests sweep 1,000 grazing directions and verify continuity,
valid ownership, unchanged ordinary normals, and invalid-input rejection.
Background on shading/geometric hemisphere mismatch:
https://pbr-book.org/3ed-2018/Materials/BSDFs . The bounded normal blend is our
chosen approximation, not an implementation of that book's BSDF formulation.

The corrected primary view normal is packed into the previously unused raw
roughness DWORD (+44), with a validity bit; packet size and bindings stay fixed.
Reconstruction history rejection and RR normal guides consume that same normal
only for validated PT pixels. The RR guide no longer zeros mode-3 specular for
PT-owned pixels now using GGX. Native fallback still follows its original rule.
The WARP guide test covers corrected normals, converted mode-3 specular, black
albedo, material mismatch, invalid data and PT on/off/admission transitions.

## Projected decals

Two further captured lighting programs now compose the PT lobes with their
original projected decal color, opacity, clipping and emission. Their lighting
combine packs RGB in YZW; the adapter rebases the source color and result mask
explicitly. There are now seventeen reviewed composition profiles. The original
emissive decal add remains after lighting selection. This does not import decals
as independent emissive geometry or recover decal layers at secondary ray hits.

Shader reflection now requires the global pixel material-texture table only
when actually used. Decals use ordinary space-zero texture slots and global
samplers, so an absent unused material array must not prevent their PT draw.
Unknown or stripped reflection remains conservative. A fourth GPU fixture tests
the YZW layout, along with explicit table-usage reflection tests.

## Remove obsolete native environment admission

The host no longer waits for an environment-composite dispatch or snapshots its
probe descriptor, ambient intensity, GI constants or submission state. Both
private primary variants return zero for environment misses and never consume
those inputs. Previously, absent or changed native environment data could cancel
an otherwise valid PT producer and restore native composition. The provider
adapter now authenticates only the three actual light counts and camera, and
zeros the unused ambient lane. Removed the unused GI CBV and scratch UAV setup.

Full-surface PT also no longer requires or revalidates native output UAVs that
it does not read. The legacy additive variant retains those requirements. Actual
PT input descriptors, bound camera/light snapshots, scene proofs, queue identity,
submission ordering and fence lifetime checks remain required. This is a host
admission fix, not a claim that missing glass or scene lights are implemented;
in-game visual validation is still required.
