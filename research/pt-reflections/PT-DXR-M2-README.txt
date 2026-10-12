PT DXR GPU TEST M2

Standalone GPU test of the new private ray-tracing pipeline. It does not launch,
attach to, modify or install anything into Control. No PIX capture is required.

1. Extract this package into its own folder.
2. Run Run-PT-DXR-Test.cmd.
3. Send GPU-Test-Results.txt back. It should finish in seconds; the GPU wait has
   a 30-second timeout. No administrator access or Python installation is needed.

The test builds a small scene and checks eight rays: opaque hit, miss, transparent
alpha cutout, solid alpha hit, short shadow segment, near clipping, invalid ray,
and invalid material. Successful hits check distance, UV and shading normal.
Both 16-bit and 32-bit native-style index records are exercised. Alpha visibility
uses the same scene-hit shader source developed for the future Control adapter.

PASS establishes these synthetic-scene DXR cases on the selected adapter. It does
not establish Control scene compatibility, full path tracing or image quality.
SKIP means no compatible hardware DXR adapter was available; it is not a pass.
CI may report SKIP or execute on a Microsoft reference adapter. Neither validates
RTX hardware. The normal launcher requires a hardware adapter; the CI-only
--allow-no-dxr option permits reference execution or an explicit SKIP.

The GPU test uses private state objects, shader tables, resources and a tiny
synthetic scene. It renders no window and writes the results to a text file.
Do not copy any files from this package into the game directory.

Host-side record limits are checked against fixture allocation sizes. Root SRVs
do not provide buffer-view lengths, so the shader uses those supplied byte limits
instead of querying dimensions from a root buffer binding. Native scene resource
admission and full geometric-normal handling remain future integration work.
