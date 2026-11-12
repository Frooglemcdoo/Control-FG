# R428 current checkpoint — 2026-11-12

Latest validated integration: 4fc555cb9d9911650e29dc14ded4a2fecd303372.
Windows core run43242659085 and FULL DLL run43242658970/job131267084977
both SUCCESS. Its artifact remains R427v2 diagnostics, NOT an enabled R428.

This continuation fixes incomplete-hit shading on phase-B rejection, carries
producer readiness, and requires owned full-shader mode for uniform-complement
recovery. Portable policy/retirement tests pass. New Windows validation pending.
Fatal rejection suppresses the failed invocation's GI6 and latches replacement
off; it does not guarantee a clean failed frame. Do not describe it as full
recovery. Only an actually recorded complement permits normal GI6 afterward.

No enabled R428 visual build has been delivered. Automatic full workflow still
packages R427v2 diagnostics. Do not relabel that artifact as a noise fix.
No measured FPS or hair/jaw improvement. Existing R425–427 captures suffice for
ordering; do not ask for another duplicate phase/heap capture.

## Completed this continuation

- Registry acquisition validates actual backing, complete native view bounds,
  wrapper identity/state and aliases under the native retirement lock.
- Frozen-render source-only startup installs creation/release tracking while
  leaving legacy AS/instance/barrier diagnostics off.
- Original-ray CPU writer accessor snapshots before the native writer scheduling
  callback, which occurs AFTER the original ray call. Exact D3D audit saved.
- Whole-invocation preflight counts all nine real committed buffers and growth
  overlap before allocation. Shared reservation covers preparing, active and
  retired idle storage; owner destruction releases it after GPU members.
- Existing four R306 binding captures imply59.62–180.47 MiB union lower bounds.
  Actual packed bytes/copy count and GPU duration are unknown. Full sample hashes
  and limits are in hit-copy-lower-bound-r428.json.
- Private source transitions are prebuilt and batched into at most two barrier
  API calls per invocation; separate source CopyBufferRegion calls remain.

## Still required

1. Complete the native scene/material/AS/descriptor borrowing contract with the
   existing preparation gate and original native retirement/heap identities.
   Source copies solve mutable hit-root geometry, not every referenced resource.
   Do not set sceneAndDescriptorLease just because pointers/frame match.
2. The live original-ray/GI6 coordinator and retired owner cache now exist in
   410adf1. They remain dormant pending installation of the independent contract.
   Rejection no longer runs GI6 on half-complete hits. Fatal failure still
   cannot reconstruct a valid current-frame image; it disables future replacement.
3. Scene adapter Windows CI passed. Its negative fixture checks
   no-reader, changed frame/fence/epoch, wrong invocation and wrong scene.
4. Confirm full DLL compilation, then enable/package the actual visual path.
   Current package is NOT that build. No new user capture is needed.

## Local evidence and source

Repo: /workspace/scratch/bb222ddfca4b/control-live-r428
Branch: work/gi-replacement-binding-r426, Frooglemcdoo/Control-FG.
Correct binaries restored:
- ../evidence/d3d_rmdwin10_f.dll SHA cceb99cb... (correct exact build)
- ../evidence/renderer_rmdwin10_f.dll SHA ebfb5b47... (correct exact build)
- ../evidence/Control Epic Compatiable/rl_rmdwin10_f.dll SHA a79234e4...
The D3D copy INSIDE Control Epic Compatiable is a DIFFERENT hash; do not use it.
Set PYTHONPATH=../pydeps for pefile/capstone. Use bash with login=false.
Library helpers restored in ../library-current. Source is committed remotely.

Useful files: d3d12_native_hit_snapshot.h, d3d12_live_scene_owners.h,
d3d12_frozen_invocation.h, frozen_allocation_budget.h, gi_native_job_runtime.h,
gi_frozen_lifetime.h, r428-integration.md and exact native audit JSON/scripts.
