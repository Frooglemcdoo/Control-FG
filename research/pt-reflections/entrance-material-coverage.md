# Entrance material coverage and replacement contract

## Result

Across the six exported views, 150 PSO references use 17 distinct pixel shaders
that declare both native light buffers. All 17 instruction streams now decode.
This is shader coverage, not pixel coverage, and does not inventory materials
that do not declare those buffers.

| Binding family | Unique shaders | Current admission |
|---|---:|---|
| General surfaces | 9 | Exact bytecode boundaries reviewed offline; runtime disabled |
| Character-light-rig consumers | 6 | Separate semantic review required |
| Decal-color-map consumers | 2 | Separate semantic review required |

The classification is based on bindings. It does not establish that every general
surface PSO is opaque: blend/depth state and material semantics remain admission
requirements. In particular, the instance-controlled alpha variant must not be
automatically treated as an opaque wall.

## Nine reviewed candidates

Full hashes, operands and captured uses are in entrance-material-audit.json and
entrance-surface-boundaries.json. Short hashes below are display labels only.

| Shader prefix | Surface combine DWORD | Fog begins DWORD | Observation |
|---|---:|---:|---|
| 7518cf8fa5be | 466 | 475 | Material factor times base texture |
| c2b522405006 | 626 | 635 | Blended material color before light application |
| 11f146abfdc0 | 544 | 553 | Detail-texture modulation before light application |
| f4b9c2b4dfee | 495 | 544 | Separate texture-based emission added at 535 |
| de308e5501bd | 1351 | 1360 | Longer preamble with immediate constants |
| 77ee9ce88a8d | 742 | 751 | Blended/detail material color |
| 3755ae5f574c | 440 | 449 | Texture-weighted diffuse; no explicit output alpha write |
| 68b095104f50 | 1325 | 1334 | Longer preamble; no explicit output alpha write |
| de9416928ae4 | 614 | 623 | Instance-controlled alpha and alternative material offsets |

Each reviewed combine is a MAD forming material-weighted diffuse plus specular.
The register arrangement differs; use the recorded exact operands rather than
assuming one register number or one universal material-color offset. The first
four reviews alone include material reads starting at byte 48 versus byte 52.

## PT producer/consumer interface

At the reviewed combine, replace only RGB with fully material-weighted reflected
surface radiance: direct plus indirect, diffuse plus specular. Exclude primary
surface emission from this input when retaining the native emission addition.
Secondary-hit emission still belongs in transported radiance. Exclude fog and
participating media, since the retained shader applies them afterward.

Preserve the original alpha and all other live registers, coverage behavior,
material sampling and atmospheric tail. In f4b9c2b4dfee the native emission
addition at 535 follows the combine at 495 and must execute exactly once. A
replacement inserted after that addition requires a different emission contract.

This contract does not authorize injecting an arbitrary radiance texture yet.
Remaining runtime requirements are screen/sample coordinates, exposure units,
texture lifetime and synchronization, exact root binding ownership, and masking
pixels whose PT result is invalid. Keep native output for unadmitted variants.
Never infer a default material or erase native illumination on a failed match.

## What the decoder fix changes

Four material shaders contain CUSTOMDATA immediate constant blocks. Those blocks
store their length in the second DWORD, unlike ordinary instructions. The decoder
previously interpreted them as normal opcodes and failed. It now preserves class
and raw payload words, advances by the declared block size and rejects truncated
or out-of-bounds blocks. This is parser support, not shader modification.

Verification: four regression tests cover payload preservation/next-instruction
alignment and malformed lengths; all pass. Re-running the inventory and audit
successfully decoded all 17 captured variants and verified the nine exact combine
contracts. No image quality, runtime compatibility or performance claim follows.

## Reproduction

```text
python research/pt-reflections/tools/map_entrance_pipelines.py ALLSOURCE.zip SHADERBLOCKS.zip inventory.json --shader-dir shader-cache
python research/pt-reflections/tools/audit_entrance_materials.py inventory.json shader-cache material-audit.json
python -m unittest discover -s research/native-integration-audit/tools -p test_decode_dxbc.py
```

No new capture is needed. Next work is the producer-side primary integrator and
the live composition adapter, with the above admission requirements enforced.
Character and decal handling remains explicit rather than silently covered by
the general-surface route.
