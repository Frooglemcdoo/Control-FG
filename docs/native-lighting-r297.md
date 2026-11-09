# R297 — indexed geometry history

Close Control, extract beside Control_DX12.exe preserving folders, then run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
load the usual save and move through several rooms for about two minutes. Exit
normally and send the automatically packaged diagnostics ZIP. No capture buttons,
zoom, fixed camera or GI RAW setting is required.

R296 completed all 32 GPU index samples with zero copy failures, out-of-range
indices or padding failures. All 14,718 old-planner rejections observed before
metadata tracking stopped were source-end DWORD padding, not dispatch limits.
The 16 rejected-range samples passed the exact uint16 path. Samples can repeat
geometry; these are not 32 distinct meshes or proof of the whole scene.

Later the historical metadata ledger filled its 8,192 address slots and safely
stopped tracking. R297 replaces its linear array lookup with a heap-backed ordered
address map and raises the hard limit to 65,536 records. No historical record is
evicted by age: old records can still be copy/update sources. Only explicit
invalidation reclaims an entry. This fixes the observed capacity and lookup cost;
it does not establish native object destruction or active scene membership.
Allocation failure and the new hard limit still reject tracking. Snapshots remain
fence-gated, immutable copies; allocation failure cannot publish a partial copy.
The GI_LEDGER_R297 marker reports actual occupancy. Probe admission stops after a
metadata fault rather than filling its retirement table with new empty records.

The same bounded R296 index sampler remains enabled by this launcher: at most
32 samples, 256 KiB each, 128 MiB source/output/readback allocation reservations.
The existing buffer inventory and binding references have separate costs. The
new history capacity is a CPU metadata limit, not a VRAM allocation. Historical
geometry descriptors also consume CPU memory. No native lighting or DirectStorage
settings change. Replacement GI stays disabled; no speckle fix is claimed.
