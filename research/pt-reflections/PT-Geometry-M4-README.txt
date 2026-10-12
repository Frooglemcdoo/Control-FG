PT geometry adapter test M4
Extract files together and run Run-PT-Geometry-Test.cmd.
Send Geometry-Test-Results.txt. Control does not need to run.

The M2 and M3 cases passed on the user's RTX 5090. This next adapter reads a
separate float3 position stream, reconstructs triangle hit positions and normals,
and derives secondary-ray origins from geometric error bounds. The private SBT
record and payload are expanded. The host admits allocation ranges and fails
closed on unsupported or unverified layouts. It does not modify native Control
state objects or infer matching buffers from capture resource IDs.

Runs all eight path cases on very large triangles (the scale that exposed the
M3 fixed-offset limitation), then translated geometry, then mirrored nonuniform
instance scaling. A single launcher produces one log. CPU tests cover native-style
R16 offsets, allocation boundaries, stale epochs, and unsupported layout rejection.
These are synthetic geometry adapter tests, NOT game integration or full PT.

Origin calculation uses NVIDIA SelfIntersectionAvoidance.hlsl, BSD-3-Clause.
See NVIDIA-SIA-LICENSE.txt. Reference:
https://github.com/NVIDIA/self-intersection-avoidance

Native position/attribute index correspondence, live resource lifetime ownership,
static BLAS metadata, material/light adapters, general shading-normal handling,
emitter MIS and final composition remain integration work. The provided capture
export exposes some dynamic BLAS inputs; serialized static BLAS data is not proof
of vertex-buffer or material correspondence. No new PIX capture is needed for
this adapter test.
