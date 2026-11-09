# R273 — GI binding audit

Purpose: identify the actual native GI shader variants dispatched in the
affected scene and the root-signature layout attached to those shaders. This
is the binding prerequisite for the planned per-light GI trace. It does not
capture per-light radiance, attach a GPU trace buffer, or fix speckles yet.

Install the contents of ControlTOH-R273.zip beside Control_DX12.exe, preserving
folders. Close any running game and launch
`ControlTOH/Diagnostics/Start-GI-Binding-Audit.cmd`.

Load the original plant and desk-lamp area with the lamp intact, RT indirect
lighting and RR enabled, Native Blend selected, and FG off. Remain in gameplay
for 15 seconds, then exit normally. Do not click CAPTURE RR. After exit the
launcher packages the logs; send every ZIP in the opened output folder.

The launcher disables the R271 hit-array capture option and restores full native
GI light contributions. The audit does not replace shaders, change descriptor
tables, allocate GPU readback buffers, or wait for a GPU fence. Normal launch
leaves this audit disabled. R272's capture recovery fixes remain included.

The log records:

- `GI_BIND_R273_ENABLED`: this diagnostic is active.
- `GI_BIND_R273_ROOT`: shader/root hashes and root layout counts, or an explicit
  failure if the root bytes were not observed.
- `GI_BIND_R273_RANGE`, `CONSTANTS`, `DESCRIPTOR`: root parameters, spaces,
  register ranges, visibility and raw append offsets.
- `GI_BIND_R273_ROOT_COMPLETE`: all bounded root parameters were reported.
- `GI_BIND_R273_DISPATCH`: observed native GI variant, group counts, command
  list, present number and invocation count. Logging is bounded.

Pipeline creation alone does not prove a shader was dispatched. The collector
must contain dispatch records for the variant being investigated. Dispatch
records do not prove descriptor contents or GPU completion. These limits are
kept explicit; this build must not be described as the complete GI light trace.

The per-light instruction contract has been independently resolved for all
eight pinned native shaders, including zero-shadow early exits and conditional
cookie sampling. A private trace buffer can only be attached after validating
the GI binding layout; the primary spotlight capture's layout is insufficient
evidence for that operation.
