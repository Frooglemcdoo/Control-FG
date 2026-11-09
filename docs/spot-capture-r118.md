# R118: read-only hit-capture setup audit

This is a prerequisite test for exact hit capture, not a lighting fix and not
yet a capture of the intersected geometry. The R117 diagnostic modes and shaders
remain available, with their existing meanings.

## Test

Close Control to install. Copy the package beside Control_DX12.exe, preserving
the NVIDIA runtimes and ControlFGStreamline directory. Load the affected room
with native RT Contact Shadows enabled and game RT Samples Count at 1. Keep FG
and glass enhancements off. Select BOTH, return to gameplay for 15 seconds,
then SAVE LOG. Send the Lighting-R118 log; new screenshots are unnecessary.

## What this records

- The native spotlight global/local root parameter layouts and payload size.
- Whether the live 96-byte hit records reside inside an observed upload buffer.
- Four sampled shader-table records, including their identifiers and unchanged
  material arguments; two repeated CPU reads check those samples for instability.
- Shader identifiers for the two native spotlight hit groups, when available.

This distinguishes a CPU-readable native table from one requiring GPU copying.
The log explicitly says `gpu_hit_capture=0`: it does not contain actual ray-hit
IDs, does not prove the entire table stable, and does not identify the blocker.

## Bounds and behavior

Only upload buffers initially in GENERIC_READ are retained, with a 32-buffer /
512 MiB ceiling. This temporarily extends the lifetime of existing upload memory;
it does not allocate 512 MiB of new buffers. The existing diagnostic worker drops
those references after a successful sample or 64 failed dispatch attempts.
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
