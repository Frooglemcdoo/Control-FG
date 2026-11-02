# R289 — GI geometry integration test

Extract beside Control_DX12.exe preserving folders, then run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep your usual settings with
RT indirect lighting enabled. Move around for about one minute, exit normally,
and send the automatically packaged diagnostics ZIP. No capture hotkeys needed.
Launch normally to leave the observer disabled.

R289 classifies submitted lists before enforcing the single-AS-queue guard.
Known empty direct/compute journals and copy queues no longer invalidate it.
Unknown direct/compute lists, genuine AS work on a second queue, concurrent AS
submissions and multiple AS-bearing lists in one batch still fail closed.
No native submission is reordered or split; no waits are inserted.

Geometry input candidates observed in shader-read/index-buffer states outrank
unclassified default-heap buffers. Known acceleration-structure storage and
readback buffers are excluded from this geometry-input inventory. Refused
admission no longer evicts useful entries before discovering it cannot fit.
The limits remain 2048 retained objects / 512 MiB logical extents. These references
do not establish stable contents, heap ownership, aliasing or residency.
Candidate state is a retention hint, not proof of an AS input or execution state.

Journals still grow to a 65536-event ceiling. GI_METADATA_R289 logs include
protected/candidate bytes, admission refusals, excluded observations, ignored
copy/empty submissions and AS-bearing submissions. Counters after a fault are not
validated scene coverage. Reports are automatic and bounded.

Replacement GI remains disabled. No GPU geometry bytes are captured, no speckle
fix is claimed, and production lighting/DirectStorage settings are unchanged.

Primary references checked 2026-11-02:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_build_raytracing_acceleration_structure_inputs
