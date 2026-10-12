# Entrance composition contract: native material shading

## Verified shader arithmetic

Overview PSO 6362 pixel shader has SHA256
`7518cf8fa5becc5bef03f7e3b0a87f910c1c93ce7e178e81413f6402da96b939`.
The focused DXBC decoder successfully parsed its 174 instructions. The evidence
excerpt is in entrance-material-composite-proof.json (offsets are DWORD offsets
in SHEX, not source lines).

At offsets 408 and 416 the shader loads diffuse t0 and specular t1. At 424 it
loads material data at byte 52. At 446 it samples material texture index
`materialID * 35`. At 459 it multiplies those material values and at 466 computes:

`surfaceRGB = diffuseBufferRGB * materialTextureAndFactorRGB + specularBufferRGB`

Therefore these two buffers do NOT have interchangeable radiance semantics.
Putting fully material-weighted diffuse PT radiance into t0 would multiply the
material factor twice in this shader. This observation applies to this shader
variant, not automatically every character, decal or transparent material.

At 475 onward the shader computes distance/height atmospheric attenuation using
atmosphere constants. The ambient SH t2 reads at 880..968 contribute inside that
atmospheric calculation. At 1212 it combines surface lighting with that fog.
At 1302 it samples participating media t3; at 1315 it computes:

`outputRGB = foggedSurfaceRGB * mediaAlpha + mediaRGB`

The separate ambient binding must not be equated with an extra surface GI term
and disabled wholesale. Preserve these atmospheric operations when replacing
surface lighting. This refines the earlier resource-name-only hypothesis.

## Overview resource path, static candidate mapping

The supported root layout has five CBV descriptors, then 36 SRVs, then eight
UAVs. The tracing tool validates that layout from exported root signatures.
The overview primary compute table begins at 367676 in exported heap 4918:

| Output | Descriptor | Resource |
|---|---:|---:|
| diffuse u0 | 367717 | 3557 |
| specular u1 | 367718 | 3553 |
| single-spot metadata u2 | 367719 | 3415 |

Source locations in CommandLists_000.cpp:

| Line | PSO | Observed use |
|---:|---:|---|
| 32017 | 6340 | Primary lighting writes diffuse, specular and single-spot outputs |
| 32335 | 6356 | Another lighting dispatch binds the same diffuse/specular UAVs and single-spot inputs |
| 32427 | 6357 | Additive ONE/ONE RGB draw from resource 3411 into diffuse 3557 |
| 32597 | 6359 | SSS shader reads diffuse 3557 into intermediate RTV 3421 |
| 32678 | 6359 | SSS draw targets diffuse 3557 |
| 32826 | 6362 | Material draw reads diffuse 3557, specular 3553 and writes RTV 3071 |

These are source locations, not asserted GPU submission order. The material
shader's diffuse/specular/atmosphere arithmetic is stronger evidence than a
resource-name inference. Additive and SSS contributors mean that a replacement
inserted only at PSO 6340 is not the final diffuse result.

## Cross-view descriptor correction

The initial trace used only `Descriptors_*.cpp` and missed the exported
`ModifyDescriptors_*` calls. It therefore resolved some draws against stale
initial heap contents. `replay_descriptor_events.py` now applies modifications
in `RenderFrame` replay order and resolves each list at ExecuteCommandLists,
including changes made after command-list population. Null and non-SRV writes
invalidate the previous descriptor rather than retaining it.

All six views now have distinct diffuse, specular and SingleSpot outputs. The
impossible instance/material-buffer associations from the initial trace are
removed. `entrance-submitted-bindings.json` supersedes the initial resource
candidates. Shader-declared null/absent bindings are still reported; declaration
alone does not prove an executed load. The original native GPU dependency graph
is not established by replay order: PIX injects queue flushes and comments out
native Wait calls. Resource values remain outside this source-only export.

## Concrete integration decision

For opaque entrance materials, introduce the new PT surface result at the
material composition boundary, BEFORE the native fog/media operations. This
avoids inverting a near-zero albedo or pretending fully weighted radiance is the
native diffuse coefficient. Match shader identity and validated live bindings,
not capture-local IDs. Keep native material alpha/depth/MSAA behavior.

Initially retain upstream native passes for compatibility consumers. Only skip
their work after tracing all consumers, especially SSS, decals, character rigs,
transparency and the SingleSpot side output. Retaining upstream work costs time
but does not imply adding its surface lighting into pixels whose composition has
been replaced. Do not claim a performance improvement before measuring it.

Next implementation prerequisite: enumerate the material variants and identify
their surface/fog split; validate the live output and guide formats. The current
reflection-only substitution is not that integration. No playable full-PT build
or visual parity claim is made by this audit.

## Reproduce

First run map_entrance_pipelines.py to produce the stage-aware inventory, then:

```text
python research/pt-reflections/tools/trace_entrance_lighting.py ALLSOURCE.zip inventory.json lighting.json
```

This uses the existing source archive only. No new game run is required.

## Offline bytecode adapter implemented (October 12)

`pt_surface_composition.h` now patches the nine SHA256-reviewed general-surface
variants listed in `pt_surface_profiles.in.h`. The caller must verify the original
SHA256; the adapter additionally checks the original container checksum, size,
shader model, instruction boundaries and reviewed instruction shapes. Unknown,
corrupt and unsupported inputs leave the output empty.

The adapter adds a float4 t35 declaration and one temporary. It loads the private
PT texture using the native diffuse-light load's original integer coordinates,
before the native code reuses that coordinate register. Immediately after the
reviewed diffuse-times-albedo-plus-specular instruction, a conditional move selects
PT RGB when its alpha admission mask is nonzero. Native alpha zero falls back to
the exact original surface term. The producer must emit only mask zero/one and
finite RGB, clear uncovered pixels, and exclude primary emission.

All native instructions remain byte-identical except the incremented temporary
count. Input/output signatures and retained non-code chunks are unchanged. Stale
reflection and statistics chunks are removed. A retail DXBC checksum is generated
using the documented Microsoft final-block layout; the implementation matched all
160 reconstructed shader containers from the existing six-view capture.

The local nine-variant patch test passed. Independent token comparison confirmed
exact preservation of the original instruction stream, with only one declaration,
one load, and one conditional move inserted. This includes the two variants whose
integer coordinates occupy r1 instead of r0 and the variant with primary emission
after the combine. Captured shader binaries remain local and are not committed.

This is not yet a native graphics PSO/draw hook. Descriptor staging, per-frame
output ownership, producer-to-compositor queue ordering, target dimensions and
raster sample compatibility must be established by the live renderer before
activation. Character/decal shaders remain excluded. Offline patch success does
not establish D3D12 PSO creation or in-game image correctness for these binaries.
