# R120: read-only hit-capture setup audit

This is a prerequisite test for exact hit capture, not a lighting fix and not
yet a capture of the intersected geometry. The R117 diagnostic modes and shaders
remain available, with their existing meanings.

## Test

Close Control to install. Copy the package beside Control_DX12.exe, preserving
the NVIDIA runtimes and ControlFGStreamline directory. Load the affected room
with native RT Contact Shadows enabled and game RT Samples Count at 1. Keep FG
and glass enhancements off. Select BOTH, return to gameplay for 70 seconds,
then SAVE LOG. Send the Lighting-R120 log; new screenshots are unnecessary.

## What this records

- The native spotlight global/local root parameter layouts and payload size.
- Whether the live 96-byte hit records reside inside an observed upload buffer.
- Four sampled shader-table records, including their identifiers and unchanged
  material arguments; two repeated CPU reads check those samples for instability.
- Shader identifiers for the two native spotlight hit groups, when available.

Finding an owner confirms a CPU-readable table. A miss does not prove the table is GPU-only.
The log explicitly says `gpu_hit_capture=0`: it does not contain actual ray-hit
IDs, does not prove the entire table stable, and does not identify the blocker.

## Bounds and behavior

The initial upload cache has a 32-buffer / 512 MiB ceiling. A separate slot
retains at most one exact-range upload owner, up to another 512 MiB, observed
through GetGPUVirtualAddress after DispatchRays identifies the table. This temporarily extends the lifetime of existing upload memory;
it does not allocate 512 MiB of new buffers. The existing diagnostic worker drops
those references after a successful sample or 240 attempts spaced at least 250 ms apart (roughly 60 seconds).
The address observer is installed on the first eligible upload resource vtable.
Resources using a different vtable or never queried again may still be missed.
Capacity, budget, contention, address-query and target-match counters distinguish
these outcomes. There is no guarantee this CPU audit will find every table.
Root-signature copies are limited to 8 KiB. Table samples read 384 bytes twice,
after validating address, extent, stride and alignment. Native records are never
written. There are no added GPU commands, fence waits or per-frame readbacks.
An unsupported layout, missing owner or mapping failure is logged without
changing the native render path. No claim of full snapshot coherence is made.

The separate geometry-capture shader and decoder remain dormant development
components. Enabling them requires verified state-object linkage, descriptor
restoration and submission/fence ownership. The gameplay shader files, centered
filter, GI filter, material guides and 60% clamp are preserved.

Validation includes address-overflow and table-layout rejection, exact material
argument preservation, binary hit decoding, and a Windows WARP upload-resource
test that checks sampling leaves records unchanged and releases retained owners.

## R120 copy-path observation

CopyBufferRegion and CopyResource are forwarded unchanged. Once a native
DispatchRays supplies the target range, up to four subsequent copies covering
its start log the live destination/source heap types and byte ranges. If the
source is upload memory, four 96-byte records are sampled twice before the native
copy is recorded. Partial coverage is explicit. These are copy-time CPU samples,
not a coherent dispatch snapshot, GPU readback, or intersected geometry.

Allocation-time address observation now includes non-upload buffers, and exact
address matches log heap type before CPU readability is considered. Hooks are
bounded to 16 command-list vtables. There are no new GPU commands or waits.
The existing 60-second audit window applies; the test remains BOTH for 70 seconds.
