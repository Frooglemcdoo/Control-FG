# R428 enabled experimental build ready — 2026-11-12

Source/build commit: 0e82c15d5f7574867f51f1b01f4d2e8e05e36f85.
Windows core run43249366979/job131290639156 SUCCESS.
Full DLL/package run43249366980/job131290638737 SUCCESS.
Artifact13959631244: ControlTOH-GI-Frozen-R428.

Verified inner ZIP: ControlTOH-GI-Frozen-R428.zip, 6991147 bytes.
SHA256: 80ccfc2040520b99c314643a9f65ae4c6c801ced27e3dd357fe35201c6f2f76c.
DLL SHA256: 51e15fd14f99e56426b2b715ec063e6695caf05b9fbd5d73ac899d18bda90ed6.
Outer artifact digest, inner ZIP digest/CRC, 88 package members, expected root
DLLs, enabled launcher and runtime markers verified. Actual Windows CMD test
passed in core and package: paths with spaces, child GI flags, cleared inherited
diagnostics and parent environment preservation. Package/migration tests passed.
The earlier b0616f1 run failed only the fixture's compiler environment setup;
its reports-only artifact is NOT this deliverable.

## What is enabled

Launch ControlTOH\Diagnostics\Start-GI-Frozen-R428.cmd with the game closed.
Normal launch leaves guided rendering disabled. Existing GI OWNED/NATIVE toggle
controls requests; RR and RELAX settings are unchanged. Same configured ray
count: phase A trains from half the rays, phase B guides the remaining half in
the current frame. No cross-frame scene reuse or extra full native ray retry.

Full prepare/join gate, saved original native heap/fence lease, private geometry,
SBT/constants, actual same-queue producer/consumer submissions and final fence
plus both recording seals are connected. Retired owners can reuse allocations
inside the shared 768 MiB budget. See r428-integration.md for exact-build native
contract assumptions and limitations; this is experimental live integration,
not universal ownership proof for arbitrary external resource writers.

## Failure behavior / limits

Preparation rejection before replacement commands preserves the existing path.
An actually recorded uniform complement permits compatible OWNED full shading.
Phase-B rejection never runs full GI6 on half-complete hits. Fatal partial
failure aborts that invocation and disables replacement for the session; its
current-frame image may be stale/incorrect. Native/owned toggles cannot make a
native reconstruction basis compatible with owned rays. No recovery guarantee
is made after loss of scene/resource validity.

No in-game visual or FPS improvement has been measured. Copies/guide computation
have real cost. Existing four R306 captures imply 59.62–180.47 MiB union copy
lower bounds; actual packed size can be greater. Do not promise FPS gains.

## Next input needed

User motion comparison at the existing jaw/neck and GI-lit hair locations,
with RR on and RELAX off, plus render FPS comparison. Projector/desk/plant must
remain clean. Standstill does not reproduce the reported problem. Actual
GI_FROZEN_R428 producer/consumer log entries establish activation; menu labels
or startup alone do not. If unchanged/broken, inspect existing logs from that
same run before asking for more work. Do NOT request R425–427 ordering/heap/
scene captures again; those checkpoints are already complete.

## Source and evidence

Repo /workspace/scratch/bb222ddfca4b/control-live-r428.
Branch work/gi-replacement-binding-r426, Frooglemcdoo/Control-FG.
Correct native binaries: ../evidence/d3d_rmdwin10_f.dll (cceb99cb...) and
../evidence/renderer_rmdwin10_f.dll (ebfb5b47...). The D3D DLL inside the
Control Epic Compatiable evidence subfolder has a different hash; do not use it.
PYTHONPATH=../pydeps for pefile/capstone. Shell bash with login=false.
