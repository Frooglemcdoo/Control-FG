# Native GI phase audit

Renderer SHA256: ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433.
Shader source: supplied Control-RR-Loose-Shaders-20260912-212234-687d93d6.zip,
rt_diffusegi.obj, containers at offsets 580 (24472 bytes) and 25096 (27904 bytes).
Base64 snapshots here are the unmodified native libraries. Their full hashes are
pinned by tools/gi_long_sequence.py before any transformation.

Renderer RVA 0x11dd0b loads the frame counter at 0x8029d0; 0x11dd13 masks with 3;
0x11dd16 writes 0x802990. The alternate branch at 0x11e338 writes 1.
Registration at 0x11e3d9 resolves the name g_uTemporalFrame (string 0x62f818).
0x11e41d reads 0x802990 and 0x11e439 passes it to setProviderData via the resolved
provider at 0x1295cc0. The current-frame provider g_uCurrentFrame (string 0x62f860)
uses the unmasked 0x8029d0 at 0x11e4b7 / setter 0x11e4d3.

Both native GI ray libraries read sys_constants register 69 component 1 (1108)
and form `(temporal * DGIPassCount + DGIPass) * RTDiffuseRayCount + rayIndex`.
They multiply this index by the same three R2 constants, add blue-noise offsets,
then take fractions to generate directions and radius variation. A fixed world
noise texel and fixed geometry can therefore repeat directions when the native
phase repeats. This is conditional: camera/world noise lookup, geometry and
lighting can change. R263's 1584-frame recurrence is consistent with period four,
but was not by itself proof of a particular period.

R264 edits only the phase factor to read component 2 (1112), masked to 255.
The exact native source is restored by reversing the one edit in each library.
No shader constants, ray counts, passes, distributions, hit shaders or roots are
otherwise edited. See docs/native-lighting-r264.md for diagnostic scope/limits.

Local disassembly used official Microsoft DXC v1.9.2609 Linux release,
archive SHA256 96faadc7f5c282d2ffda49804beb4c3ee38127bc252b723234e3c5cdf7aa39a1.
Build DXIL is reassembled/validated by the Windows SDK DXC selected in CI, not by
rewriting a native container checksum. The system LLVM reader rejects the old
library's data layout; no input was changed to work around that rejection.
