# R298 — geometry layout support and rejection evidence

Close Control and extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
move through several rooms for two minutes, exit normally and send the automatic
ZIP. No capture buttons, fixed camera, zoom or GI RAW setting are needed.

R297 completed all 32 index samples without copy, index-bound or padding failures.
Metadata stopped at 5,560 historical records on blas_unsupported_descriptor. The
old log did not identify the descriptor field, so it cannot establish the cause.

R298 removes the parser's packed-12-byte-stride restriction for float3 vertices:
aligned strides of at least 12 bytes are supported and retained as supplied.
The vertex span is (vertexCount - 1) * stride + 12, with overflow checks, instead
of including unused padding after the last vertex. Ownership checks and scene
read plans use the same span. This is metadata support, not a new vertex decoder.
The existing downstream decoder remains limited to its supported layouts.

If another geometry is rejected, GI_GEOMETRY_REJECT_R298 records the first exact
reason, geometry index/type, build layout/flags/count, source/destination, vertex,
index and transform addresses, stride, counts and formats. Evidence is copied
while the descriptor is valid; no native descriptor pointer is retained. The
reason distinguishes parser support limits from range/alignment checks. A rejection
still stops metadata publication and quarantines unresolved capture owners; it
never removes geometry from the game's own AS build or changes native execution.
This build may expose another unsupported case; it does not claim the prior
failure was caused by stride.

The R297 indexed ledger and bounded R296 index sampler remain. No replacement GI
or speckle fix is enabled, and existing lighting/DirectStorage settings are kept.

Reference: Microsoft D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC documentation:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_raytracing_geometry_triangles_desc
