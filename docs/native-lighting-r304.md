# R304 — GI submission lifetime integration

This build retains observed AS build/copy resource references from recording
through submission, replay, command-list reset and completed private queue fences.
Completed submissions release their extra journal references. Failed signals or
uncertain submissions retain their references. Tracking is bounded at 256 pending
submissions; exhaustion disables the observer without suppressing native commands.

The combined offline scene consumer validates geometry, instance placement and
native material records using recovered PT code. It is not yet game-connected.

Native resource retirement and replacement GI remain disabled. Retaining build
and copy roots does not cover all BLAS traversal references or DispatchRays uses.
The existing 512 MiB AS registry cap is unchanged; this build does not claim to
fix saturation, speckles, or improve performance. No shader lighting changes.

## Optional integration run

Install the complete package beside Control_DX12.exe, preserving folders.
Run ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd, load the usual save,
move around for two minutes, then exit normally. No hotkeys or fixed camera.
The launcher packages logs automatically. GI_AS_LEASE_R304 records submitted,
completed and unresolved root counts alongside existing whole-scene metadata
and bounded geometry/instance snapshots. Launching normally leaves the observer off.

## Validation

Windows validation covers combined geometry/material admission and a real WARP
queue blocked by a fence. The latter submits and replays an empty command list,
retires a synthetic AS resource identity, resets the list's metadata, verifies
leases remain held, then releases them only after queue completion. This tests
resource lifetime, not hardware DXR execution or in-game visual quality.
