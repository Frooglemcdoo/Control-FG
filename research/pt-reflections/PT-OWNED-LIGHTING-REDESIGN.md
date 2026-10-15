# PT-owned lighting redesign — implementation checkpoint

Status: development source only. R66 runtime and install bundle remain unchanged.
No FPS improvement or complete RT replacement is claimed by this checkpoint.

## Verified cause of the lighting limitation

`pt_primary_path.hlsl` builds the deployed producer with PRIMARY_FILL and
PRIMARY_INDIRECT: no first-hit direct sample and no first-hit specular
continuation. `PTCommitIndirect` adds to native specular u1; native diffuse u0
remains unchanged. This is additive diffuse transport, not complete surface PT.

The replacement boundary is the reviewed material compositor's
`diffuse * materialColor + specular` result, before emission, fog and media.
Putting material-weighted radiance in diffuse would multiply albedo twice.

## Implemented

`pt_primary_surface.hlsl` shares the primary adapter and scene traversal but
selects complete direct + diffuse/specular continuation. It excludes primary
emission (the downstream compositor already adds that) and includes secondary
emission. The separate u7 output stores float4(material-weighted radiance,coverage).
It never reads or writes native lighting UAVs. Black radiance has coverage 1;
invalid paths have coverage 0. Extent validation rejects the old 1x1 placeholder.
Sparse readiness probes do not publish surface pixels. Whole-output initialization
and replay invalidation remain mandatory host responsibilities.

Windows CI compiles both producers independently and runs the WARP commit test
in both modes. The new shader is deliberately absent from release packaging.
The existing executable must never load this shader under the legacy filename.

## Host integration gaps found in the audit

- `FrameFor` allocates 1x1. Replacement needs the view's internal render extent.
- `surfaceTables` is read by Drawing but never allocated. Reserve its 64 tables
  in the authenticated producer allocator context, with normal retained lifetime.
- commandConsumer has null drawing/drawn callbacks and computeOnly=true.
  Restoring composition requires graphics tracking, matching-frame admission,
  descriptor validation, and post-draw state restoration as one change.
- Nine reviewed general-surface profiles do not cover characters, scattering,
  decals, or transparency. Those need explicit ownership/fallback contracts.
- Native u2 single-spot metadata and later diffuse/SSS consumers still exist.
  Do not skip their producers merely because a general-surface PT output exists.
- Native dispatch executes before PT preparation. GPU admission can arrive later.
  Unconditional native suppression would remove fallback on rejected frames.

## All-light support is not yet complete

The current adapter samples native point/spot buffers and at most one sun,
including projector textures. These are delta samples, not finite-area emitters.
Verify the buffers include off-camera sources needed by secondary hits before
claiming scene-wide coverage. Emissive geometry and environment are currently
sampled through BSDF continuation; efficient explicit sampling needs emitter
selection and MIS. Simply allowing non-delta direct samples duplicates energy.
Special materials can invalidate a whole path and fall back to native rendering.

## Performance work still required

Existing R60 traces and R63-R66 timing windows show PT ray cost scaling with
internal resolution. The fixed scene preparation path remains roughly 2.7ms.
PreviewScene reconstructs scene metadata and PrepareLease revalidates all rows.
Introduce immutable version-keyed scene/record caches with incremental dirty
updates, preserving strict submission-time version checks. Frame IDs or render
extent alone cannot establish cache validity.

The global sceneUseFence currently orders every foreign queue submission after
PT use. Native AS memory is still shared: geometry input copies do not snapshot
AS contents. Replace this only after tracking actual overwritten resource ranges
and relevant read lifetimes, or creating independent AS versions. Removing Wait
without that proof introduces traversal races. CPU wait duration does not measure
GPU serialization.

## Required release evidence

Compile and WARP checks, then host/composition admission and replay tests; prove
native outputs are not double-counted and unsupported consumers remain valid.
Only suppress identified native passes after their replacements own all outputs.
Use the existing captures for the CPU/queue redesign. No new user capture is
required to establish these implementation gaps. Do not issue a gameplay test
bundle for this source-only checkpoint.
