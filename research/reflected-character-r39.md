# R39 reflected character audit

Status: investigation, not a rendering fix. R38 remains the visual baseline.

The supplied 20261019-164209 archive contains the exact native glass hit
library and lighting shader targeted by the current enhancement. The matching
hashes are enforced by `tools/audit-reflected-character.py`. Run that tool on
the collector ZIP to reproduce the shader-binding and instance-layout evidence.
No captured game binaries are committed.

## Confirmed shader differences

The standard-material archive has 3,624 containers; 512 bind
`g_bCharacterLightRigs`. The examined primary pixel shader is SHA256
`cf68e7a8831c24b45c9c0fd54f03cb3c1defd0f99fe0bf68f0771333d888cb66`,
with 16 byte-identical copies among those variants. This establishes a native
character lighting path, but does not prove which variant rendered Jesse in
the screenshot.

Its `g_sbInstanceObjectData` has stride 64. Reflection metadata identifies the
signed `g_iCharacterLightRigIndex` at byte 44. Instructions 590 and 600 read
that member using interpolated input v5.x and compute:

    address = rigIndex * 144 + g_uCharacterLightRigsBindOffset

The shader reads three 48-byte light records. Each has a position/parameter
float4, direction/range float4, and RGB intensity. Its shading includes
view-volume lighting modulation, a BRDF, bent normals and optional translucency
kernel sampling. Adding raw light colors would not reproduce this calculation.
The signed index and byte offset must not be treated as a material ID.

The matching glass lighting shader binds neither the character rig buffer,
the instance-object buffer, nor the character-lighting constant buffer.
This is a concrete missing lighting path and a plausible contributor to the
comparison. It does not establish that it is the sole cause of missing detail.

The native glass reflection closest-hit shader already samples the base normal
map at mip zero and uses its tangent basis. Its local root constants carry
index/vertex layout and material ID, but no primary-render instance index.
The current saved hit data carry position, normal, UV and material ID, also
without a primary-render instance index. Mesh LOD has not been established as
the cause by these captures.

## Native resource trace

Renderer SHA256:
`ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433`.
D3D DLL SHA256:
`cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5`.
These offsets apply only to those exact binaries.

Renderer RVA 0x30a50 uploads the rig array, allocates count*144 bytes, writes
nine float4s per rig, then calls DynamicBuffer::unlock through IAT 0x5dfda0
at RVA 0x314ae. Rig buffer provider storage is at 0x1290a78; the binding
offset value is at 0x1290a98. The upload Slot is 0x60 bytes, with its mapped
write pointer at +0x50 and the published byte offset at +0x58.

The D3D ShaderBuffer constructor at 0x41790 delegates to setBuffer at 0x417b0.
The latter stores a descriptor handle selected from the NativeBuffer fields;
the global ShaderBuffer is NOT an ID3D12Resource pointer. DynamicBuffer::lock
is at 0x21330 and unlock at 0x215c0. A read-only diagnostic at the exact
uploader call can copy records while their source is valid; it must not retain
the borrowed mapped pointer across frames.

The opaque draw at renderer 0x1c1d26 selects an 80-byte instance-buffer record
using batch.instanceBufferIndex. That record, batch.instanceByteOffset and
instanceCount feed the mesh draw at 0x1cb100. This is not yet evidence of a
mapping from DXR InstanceID to the primary pixel shader's v5.x.

## Required before a visual build

1. Trace the primary instance index through the vertex shader and native
   instance-buffer setup, and establish the corresponding DXR hit identity.
   Do not assume DXR InstanceID equals the primary instance index, or that a
   material belongs to only one character.
2. Bind the matching view's rig records with valid GPU lifetime. A provider
   address alone does not establish view identity or freshness.
3. Port and validate the character light evaluation with its required material
   inputs. Account explicitly for inputs absent in reflected shading, including
   bent normals and translucency, rather than substituting arbitrary brightness.
4. Compare character lighting in-game while preserving R37's native-light merge
   and R38's shadow filter. Do not globally change the first-hit viewing vector.

No production shader, runtime hook or packaged binary changed in this audit.
