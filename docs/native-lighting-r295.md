# R295 — combined GI binding and lifetime integration

Extract beside Control_DX12.exe preserving folders. Close the game first, then run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
load your usual save, move around for about one minute, exit normally, and send
the automatic diagnostics ZIPs. No F7/F9, RR capture, zoom or fixed camera.

This is an integration test, not a speckle fix. The launcher enables both
CONTROLTOH_GI_METADATA and CONTROLTOH_GI_BINDINGS. Normal launching does not enable
them. Existing lighting, DirectStorage and clamp settings are preserved.

One run collects:
- Existing complete geometry-owner/local-state observations.
- Compute binding availability at every intercepted AS build, including ordinary
  compute versus raytracing PSOs and missing/incomplete binding coverage.
- Storage proof and DWORD-aligned range feasibility for vertex, index, transform
  and instance inputs. Counts are repeated observations, not unique allocations.
- Actual queue submissions, private fence completion and recording lifetime:
  completed recordings are counted only after successful Reset or destruction
  seals them against future replay.

GI_INTEGRATION_R295 and GI_INPUT_R295 are the new log lines. R294-labelled metadata
lines remain for collector compatibility. local_preflight_pass is NOT copy-ready:
resource alias activation, residency and source-byte versioning remain unproven.
Unknown state is not assumed COMMON. No GPU geometry copies, new lighting,
readback, CPU wait for GPU completion or denoising changes are added.

The lifetime monitor owns small CPU records only, with at most 256 active records
and 16 retained queue/fence identities. It polls without waiting. Native execution
always proceeds, including when diagnostic admission fails. Cross-queue replay
or failed signals quarantine a record rather than certify it. Overflow/failures
are reported and invalidate coverage conclusions. Existing geometry inventory
budgets remain; retained placed heaps can be larger than their buffer extents.
This build is diagnostic and must not be used to benchmark normal-launch cost.

The queue-fence adapter deliberately retains unresolved ownership through runtime
teardown. It does not insert cross-queue dependencies or make unsafe concurrent
GPU writes safe. The live monitor does not own rendering buffers; GPU-backed
ownership and raw copy/restoration are tested separately in Windows WARP.

Microsoft's ExecuteCommandLists contract requires prior execution to be complete
before reusing a command list. A completion fence still does not make future
replay impossible, so publication waits for recording seal as well:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists
