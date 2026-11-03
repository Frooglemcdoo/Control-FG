# R301 — geometry evidence and capture diversity

Close Control and extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
move through several rooms for two minutes, exit normally and send the automatic
ZIP. No capture buttons, fixed camera, zoom or GI RAW setting are needed.

R300 completed 41 paired samples with no copy, index-range or nonfinite-position
failures. Eight samples of one 91,799-vertex descriptor had 9,362 zero-area
triangles each; another uint32 descriptor and all sampled uint16 data were
nondegenerate. The full vertex span was not all-zero by hash comparison, but that
does not establish values of the referenced subset. No cause is yet established.

The DXR specification permits degenerate triangles and allows changing their
positions during updates. They produce no triangle intersections. NaN in a vertex
x component can represent an inactive triangle. These observations must not be
used to discard an entire mesh or renumber primitive indices.
Reference: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#degenerate-primitives-and-instances

R301 preserves the existing copies and checks, adds counts for repeated indices,
coincident positions and distinct collinear positions, and separately reports
all-zero referenced coordinates and x-NaN markers. Inactive counts are diagnostic
markers only, not a complete DXR legality check. Up to four witnesses per capture
record exact index values and float32 position bits: first, last, first degenerate
and first nonfinite triangle. Build destination/flags, geometry count/index and
source addresses associate this evidence with the native descriptor.

Sampling now limits repeated source-owner/offset/layout descriptors to two
attempts per session so later geometries can be selected on subsequent builds.
The bounded 48-key table is diagnostic selection, not a scene/lifetime cache.
Different source identities are distinct; repeated dynamic content is not
exhaustively sampled. Quotas and allocation budget can still limit coverage.
No source state transitions, blocking waits, rendering changes or blanket
zero-area rejection were added. Existing 16 MiB vertex and 128 MiB reservation
limits remain. These diagnostics do not yet capture TLAS transforms or materials.

This build does not enable replacement GI or fix speckles. The existing scene
instance/material work remains the next integration dependency.
