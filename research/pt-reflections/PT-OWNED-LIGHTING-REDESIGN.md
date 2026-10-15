# PT-owned lighting redesign — implementation checkpoint

Status: R69 recovery disables the R67/R68 surface replacement in the shipped
host and restores the R66 native-light-buffer plus PT-indirect contract.
The surface implementation remains development-only. All-material/all-light
replacement, stable reconstruction, native-pass removal and the performance
redesign remain incomplete. Historical R67/R68 details below describe those
builds, not the active R69 route.

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
in both modes. R67 packages the new shader under its distinct filename. The legacy executable
must never load this shader under the legacy filename.

## Development host route

`CONTROL_PT_SURFACE_REPLACEMENT` now selects the distinct shader filename,
allocates at internal render extent, reserves 64 retained composition tables,
checks allocator overlap, and registers graphics draw/restore callbacks. It
removes the additive path's typed native-UAV-load requirement. R67 packages this host route and separately compiles the legacy host. Graphics
tracking is configured at startup only when PT was selected for the session.
This restores the previously disconnected composition path, but is not evidence
of visual correctness, denoiser compatibility or complete material coverage.

## Host integration gaps found in the audit

- Release `FrameFor` allocates 1x1; the development route now uses render extent.
- Release `surfaceTables` was never allocated. Development now reserves its 64
  tables in the producer allocator context and retains them with the frame.
- Release commandConsumer remains compute-only. Development reconnects graphics
  tracking and the existing matching-frame admission/restore callbacks.
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
required to establish these implementation gaps. R67 is a visual integration build with those remaining limitations stated in
its README and Build.json; it is not the completed redesign.


## R67 validation scope

Local adapter test rechecked all nine captured native shader binaries, including
container checksums and corrupt/unknown shader rejection. Transport tests passed
12 numerical integration cases in the preceding checkpoint. Windows CI runs the
native forwarding/compute-only hook fixtures, output lifecycle/admission/replay,
full-surface commit and legacy commit tests, DXIL compilation, and both host
compilation routes. Final package verification must check commit identity,
complete hashes, matching PDB, distinct surface shader and preserved settings
scripts. These checks do not establish in-game lighting quality or FPS.


## R68: R67 flicker report and routing correction

Evidence: PT-Primary-M12R67-Results-20261015-162656-315.zip and
20261015-2124-42.9425473.mp4. Video frames show material-sized lighting changes.
The log has one transport-ready transition and zero PT_PRIMARY_ADMISSION or
PT_PRIMARY_LATE_REJECT records. Sampled producers stay armed and covered.
It also has surface_bindings skips, primarily in bursts; these checks are not
relaxed because the captured shaders actually consume the required roots.

R67's silent 64-table ceiling and producerRecorded checks in Drawing create
CPU-worker-order-dependent native/PT selection. Counts over 600-record windows
are consistent with the 64-table ceiling in multiple intervals, but R67 did not
log exhaustion directly; the video alone cannot attribute every fluctuation.

R68 reserves a frame/output at the earlier opaque job and retains it in Candidate.
Surface recording may precede producer recording; Selected still checks that an
armed/deferred producer precedes consumers in GPU submission order. The 1024-table
budget never wraps. Failed reservation/exhaustion cancels retained outputs and
latches native routing until restart. New logs expose reservations, usage and
routing stops. Randomized scheduling tests cover 200 draws across 40 frames;
WARP verifies a consumer recorded before its producer reads the ordered result.
No shader, RR, exposure, light sampling or scene-sync change is included.
This fixes concrete routing defects; in-game flicker resolution remains to be
verified with the same entrance view.

## R68 evidence and R69 recovery

The supplied R68 log records `PT_PRIMARY_SURFACE_ROUTING_STOP` at frame 4028,
QPC 2423678777983, reason `surface_table_reservation`, tables=0, capacity=1024.
This positively identifies the shutdown path. `Fresh` rejects a missing,
changed or out-of-bounds native heap provider; the log does not distinguish
those cases. It is not evidence of VRAM exhaustion. The 49*1024 reservation
was introduced by R68. Counts reach 169 tables in the late scene: the old 64
limit was insufficient, but reserving 1024 every frame was not a sound fix.

There is also no reconstruction between `PTCommitSurface` and the patched
surface load/select. The random seed varies with frame; the direct estimator
selects a single light. Unsupported secondary BRDF hits invalidate the entire
path, clearing coverage and selecting native lighting for that camera pixel.
At frame 3884, 1999 paths integrated, 1888 covered, 111 faulted (97 BRDF and
14 normal). These mechanisms are present in source; their individual share
of visible flicker is not established by the throttled log/video.

R69 ships the legacy host contract, retains PT indirect, and restricts early
Job frame/output reservation to the surface-development route. Recovery and
surface macros together are a compile error. Build metadata and startup logs
explicitly identify recovery; settings scripts remain unchanged. No claim
that replacement flicker is solved or full PT lighting is delivered.

Before surface replacement is packaged again:
- Separate primary material ownership from stochastic secondary-path validity;
  define supported secondary transport and failure handling without swapping
  the entire primary lighting model based on the sampled bounce.
- Implement and validate reconstruction with motion, depth/normal rejection,
  disocclusion, camera cuts and render-resolution changes; verify the exact
  relationship to native denoising and DLSS rather than assuming it filters PT.
- Replace per-draw cloned native descriptor ranges with a lifetime-safe output
  binding design that handles native heap rotation and recording concurrency.
- Validate coverage/order across the full material set before native-pass
  suppression. Do not remove fallback producers while consumers still need them.
