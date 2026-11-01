# GI instance and triangle decoder

R279's captured 512 BLAS build calls contained 674 triangle descriptors. All 674
were recorded: float3 positions (DXGI format 6), 12-byte stride, R16_UINT indices
(format 57), and no geometry transform address. The build budget was exhausted in
present 1608; no TLAS records were obtained. There were 512 compact-copy records.
The log separately reported 120 dropped diagnostic lines. None of these facts
establish a complete live scene map.

The R278-format ray trace in this session contained 25,200 records, 21,600 rays,
3,600 complete pixel sets, no overflow, and verified runtime accounting with
maximum absolute error 1.7229467630386353e-08. Its SHA256 is
4071818993311c0fd91d24cc3d5c786dd2e8e9552066d7e5f2788a7fa749959f.
The sampled-grid fallback share was 83.20%; this is not evidence identifying the
plant, hair or a specific light, nor a temporal variance measurement.

`tools/gi_instance_geometry.py` decodes an immutable 64-byte native DXR instance
record, separates the 24/8-bit fields, then reconstructs an indexed triangle from
bounded position/index snapshots. It applies geometry transform before instance
transform and calculates the geometric normal from transformed edges, retaining
winding. Unsupported formats, stale generations, out-of-range addresses/indices,
nonfinite transforms and degenerate triangles fail closed. Six unittest methods
cover field packing, geometry, transform order, reflected winding and rejection
cases. Run `python tools/test-gi-instance-geometry.py`.

This is offline scaffolding, not integrated rendering code. Generation labels
must come from verified resource/build tracking; they are not proof of GPU
completion. The caller must establish the accepted instance, geometry and
primitive and supply matching buffer snapshots. InstanceID is application data,
not necessarily an instance-array index. Instance flags and material sidedness
still require separate visibility interpretation. A decoded triangle is not
proof that a ray hit it.

## Next integration gate

Do not repeat R279 or simply increase its startup logging budget. Preserve a
bounded descriptor map separately from diagnostic text, track AS copies and
updates, and reserve independent TLAS records. Obtain same-generation instance
and triangle bytes through a fenced GPU snapshot. Capture accepted-hit identity
and validate reconstruction against native hit positions before sample reuse.
The current R279 capture establishes a layout subset only, not a world-space
triangle mapping. No production lighting changes are enabled by this work.

Reference: Microsoft D3D12_RAYTRACING_INSTANCE_DESC, read 2026-11-01:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_raytracing_instance_desc
