# R294 — command-list-local geometry state evidence

Extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd, keep RT indirect lighting on,
move around for one minute, exit normally, and send the automatic diagnostic ZIPs.
No hotkeys or fixed camera. Replacement GI remains disabled.

R293 passed the supplied run: 875984 successful build-time lookups, zero misses,
3056 completed AS submissions; all 23195 ranges in the final completed plan
resolved. The 1 GiB diagnostic extent budget remains. Inspection reported 0 ms
23 times and 16 ms once using a coarse clock; these are not precise cost timings.

R294 observes legacy ResourceBarrier transitions per command-list recording
journal and checks that local evidence at each build-time geometry lookup.
GI_LOCAL_STATE_R294 reports kind 0=vertex,1=index,2=transform,3=instances, with
unknown, explicit_nonpixel, split_pending and explicit_other counters. These
are repeated CPU-recording observations, including possibly unsubmitted lists.
Missing owners are counted by existing coverage counters, not local-state bins.

State keys use unique owner-inventory admission generations, not reusable GPU
addresses or raw COM pointers. Eviction/re-admission loses state evidence rather
than borrowing a prior object's state. Successful list reset creates a fresh
tracker. Aliasing barriers clear prior evidence. Split ends require a matching
local begin; invalid sequences stay unknown for that identity until reset/clear.
Each tracker has an 8192-entry cap; allocation/capacity failures clear evidence
and disable that tracker until list reset. No resources are retained by it.

This is NOT a global or GPU-completed state tracker. Initial states, implicit
promotion/decay, other command lists/queues, enhanced barriers, residency, placed
heap ownership and alias lifetime remain unproven. No actual state is inferred
from a missing transition. explicit_nonpixel means the last locally observed
explicit transition includes NON_PIXEL_SHADER_RESOURCE, not that copying is safe.
No GPU copies, barriers, waits, Map or rendering changes are added. copy_ready=0.
The existing 65536-event journal and 65536-range plan bounds remain.

Research: Microsoft's resource-barrier documentation explains buffer decay after
ExecuteCommandLists and the prohibition on access during pending split barriers:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
