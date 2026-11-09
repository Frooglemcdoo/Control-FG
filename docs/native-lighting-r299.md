# R299 — 32-bit geometry indices

Close Control and extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
move through several rooms for two minutes, exit normally and send the automatic
ZIP. No capture buttons, fixed camera, zoom or GI RAW setting are needed.

R298 identified a mesh with 91,799 vertices, 28,086 indices and index format 42
(DXGI_FORMAT_R32_UINT). The former 16-bit-only parser stopped metadata publication.
R299 retains each geometry's index width in ownership checks, scene read plans,
primitive offsets, GPU sample validation and the offline triangle decoder.

The existing native-16-bit-load shader copies bytes exactly: two halfword loads
reassemble one DWORD without converting the index value. 32-bit CPU validation
now decodes all four bytes. Input alignment and range checks remain fail closed.

A separate quota permits up to 16 uint32 samples in addition to the two existing
16-sample uint16 categories. The total reservation cap stays 128 MiB; reaching
that cap can still prevent later samples. Each completed sample logs index_bytes,
maximum index and range failures. Sample copies remain at most 256 KiB, with
large buffers sampled at their tail. This is bounded coverage, not every mesh.

The GPU fixture runs both widths and verifies byte hashes, index bounds, padding,
command state restoration and a uint32 value above 65,535. The captured descriptor
is a metadata regression case. The offline fixture covers padded float3 vertices,
uint32 values above 65,535, truncated data and misaligned inputs.

This build does not enable replacement GI or claim to fix speckles. Existing
lighting and DirectStorage settings are preserved.

Reference: https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
