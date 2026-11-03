# True-hit geometry checkpoint

The new shared C++/HLSL helper reconstructs the unbiased hit point from the actual
ray origin, normalized direction and RayTCurrent. It derives a geometric normal
from the accepted triangle's world-space edges, retaining winding. Scaling edges
before their cross product avoids unnecessary magnitude overflow/underflow.
Degenerate triangles, nonfinite data and nonunit directions are rejected.

Tests cover the unbiased position, reversed winding, a nonuniformly transformed
triangle, large/small scales and invalid inputs. The HLSL fixture compiles the
same implementation. This is not a live triangle decoder: the caller must still
prove that all three vertices belong to the accepted triangle in the same frame.
`valid` means the supplied arithmetic inputs are usable, not that identity,
visibility, barycentric membership or a native binding was verified.

## Missing native binding

The audited GI closest-hit library exposes the index buffer and vertex-buffer-1
attributes. Its local constants describe index offset/stride, vertex stride,
normal/tangent/UV/color offsets and buffer sizes. They do not establish a position
attribute layout. It would be incorrect to infer vertex positions from normal or
UV offsets, or claim that positions cannot exist elsewhere in the resource.

The portable integration route is to associate the acceleration-structure build's
triangle descriptors with the native hit identity: BLAS build/update generation,
geometry index, primitive index, instance transform, geometry transform, vertex
format/stride and index format/base. Copy/compaction and resource lifetime must be
tracked before a GPU position load is permitted. That adapter is not implemented
by this arithmetic helper. Repository production code currently has no verified
position-stream mapping for this GI path.

## New API research

Microsoft's [TriangleObjectPositions proposal](https://github.com/microsoft/hlsl-specs/blob/main/proposals/0041-triangle-object-positions.md)
describes direct access to triangle vertices, but specifies Shader Model 6.10 and
acceleration structures built with ALLOW_DATA_ACCESS. At review time its status
is Under Review. It is not a drop-in call on Control's existing libraries or
unverified BLAS builds. No device capability, SDK upgrade or build-flag change was
made here. See also the [DXR specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html).

## Range estimator

The earlier randomized-distance finding remains a separate requirement. This
geometry helper does not turn the conditional cosine-direction density into a
complete proposal for finite-radius GI, and does not alter native miss fallback.
The current reuse gate therefore remains disabled until the producer adapter,
range accounting, destination shading and fresh visibility are established.

No game shader is replaced and no new user capture is requested at this stage.

## Accepted-hit GPU producer

`gi_producer_geometry.hlsl` uses the existing recovered PT private-record ABI:
index SRV t0/space3, attribute slot t1 retained for native material handlers,
position SRV t2/space3, and the twelve admitted local constants. It reads the
accepted primitive's indices and actual float3 positions, applies the instance
object-to-world transform, derives a winding-preserving geometric normal from
world-space triangle edges, and returns `O + D * RayTCurrent` without the native
0.01 position shift. This reuses `GIRecoverTrueHit` and the earlier PT table/geometry
admission contracts instead of interpreting interpolated shading normals as faces.

The payload is 64 bytes with a reserved first DWORD for the native GI payload
prefix. This is NOT by itself proof that a native any-hit export can be linked:
the host must use the admitted export, its material resources and matching local
root. A native 96-byte record cannot be used directly; the existing private table
converter supplies the extra position binding. Geometry transforms must be absent
as already required by PTSceneAdmission; no transform is silently omitted.

The shader requires Shader Model 6.5 for GeometryIndex. The eventual host must
check that capability before creating/selecting it. DXC compilation passed;
actual producer traversal and native alpha linkage are not yet executed or bound
in Control. Existing 5090 visibility testing validates the separate four-byte
visibility payload, not this new producer payload or geometry reads. No repeat
of the earlier test is requested for that unrelated contract.
