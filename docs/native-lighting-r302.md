# R302 — native scene-instance array capture

Install with Control closed, preserving folders beside Control_DX12.exe. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
move through several rooms for two minutes, exit normally and send the automatic
ZIP. No capture buttons, fixed camera, zoom or GI RAW setting are needed.

R301 showed distinct indices referencing zero positions in portions of one mesh;
its middle geometry also contained nondegenerate triangles. Do not reject the
whole mesh or renumber primitive indices. Intentional collapse versus producer
readiness remains unresolved. Replacement GI is still disabled.

R302 adds a separate quota of four TLAS instance-array snapshots, maximum 4 MiB
each and 32 MiB total conservative allocation reservation, alongside existing
geometry sampling. It accepts observed committed DEFAULT or UPLOAD buffers from
the same device, reads under the native AS input contract, retains resources before
commands and waits for recording seal plus GPU completion before CPU decoding.
No source transitions or blocking waits are added. Placed/unknown buffers,
conflicting states, unsupported array layouts and excessive sizes are refused.

All captured records are decoded for a whole-array byte hash, counts, masks,
null BLAS references, alignment, finite transforms and unknown flag bits. The
first eight records additionally log array index, application InstanceID, mask,
BLAS address, transform, flags and hit-group-table offset. Array index and
InstanceID are not interchangeable; a hit-group offset is not a material ID.
These are structural diagnostics, not a full DXR legality validation.

The snapshot does not yet associate BLAS references with immutable geometry at
the same submission cut, establish native accepted-hit identity, or validate
materials. Logging a BLAS address is not proof that current metadata at that
address belongs to this capture. This build does not enable replacement GI.

Tests decode packed fields/transforms and null/masked/invalid records. A Windows
GPU fixture copies a real upload-heap instance array alongside a geometry sample,
checks its exact hash after fence/reset, and checks native compute restoration.
Existing 16/32-bit and paired geometry fixtures remain required.
