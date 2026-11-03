# R300 — paired index and vertex capture

Close Control and extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
move through several rooms for two minutes, exit normally and send the automatic
ZIP. No capture buttons, fixed camera, zoom or GI RAW setting are needed.

R299 completed 45 index samples, including 13 uint32 samples, with no range or
padding failures. Metadata continued through 91,271 builds without rejection.
These are sample completions, not 45 distinct meshes: repeated content occurs.

R300 pairs each sampled index window with the full corresponding float3 vertex
span at the same native AS build boundary. It preserves packed or padded stride,
16/32-bit index width, geometry order and triangle-aligned tail windows. Both
copies retain source ownership before commands, restore native compute state,
and share the same recording lifetime and completion gate. No source transition
or CPU wait is introduced. Partial pairs are not reported as valid triangles.

Full vertex spans are limited to 16 MiB per sample; index windows remain at most
256 KiB. All source/private allocation charges count toward the existing 128 MiB
reservation cap. Reservations are conservative and do not refund. Oversized,
unknown, placed or conflicting-state sources are skipped. This is bounded sample
coverage, not a complete scene snapshot. The separate observer owner cache still
has its existing 1 GiB cap; R299 reached this cap without missing geometry ranges.

GI_TRIANGLE_COPY_R300 logs vertex byte hash, stride, geometry index, first sampled
primitive, triangle count, invalid indices, nonfinite positions and degenerate
triangles. Degenerate triangles can be intentional geometry; their count alone
is not treated as corruption. Validation uses object-space positions. Geometry
transform presence is logged, but transforms and TLAS instances are not applied;
this does not prove world-space hit identity or material correspondence.

Tests exercise both widths, uint32 indices above 65,535, padded stride, exact
vertex extent, truncated buffers, invalid indices, NaN and degenerate positions.
The paired GPU fixtures compare exact vertex and index hashes and require one
valid triangle after fence completion and Reset, with native state restoration.

This build does not enable replacement GI or claim to fix speckles. Existing
lighting and DirectStorage settings are preserved. Launch normally to disable
the diagnostic observer.
