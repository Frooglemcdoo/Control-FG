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

## Cross-view limitations found

All six shader inventories parse, but the exported descriptor snapshots do not
yet form six independently verified binding graphs. Some shader-declared
descriptors are absent/null. Several other views resolve distinct primary
outputs to the same exported resource; some resource-role matches are implausible.
The tool records these as candidates and explicit aliases/unresolved bindings.
It does not hide them or claim successful replay validation. The cause could be
export/snapshot semantics or unsupported tracing state and is not established.

The overview's inspected material/light path is useful evidence; arbitrary
matches elsewhere in entrance-lighting-candidates.json must not become runtime
disable rules. Resource IDs and descriptor offsets are capture-local.

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
