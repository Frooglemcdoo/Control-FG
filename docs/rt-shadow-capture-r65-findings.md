# R65 capture findings

Source: `Lighting-R65-20261021-105124-13024-0.log`.
All 22 blobs extracted with matching lengths and SHA256 hashes (477,776 bytes).
Expected, created, saved and bound compute masks were all 2047; missing mask was
zero. No capture limit was hit. These are CPU-side observations, not proof of
GPU completion or of complete shadow-caster coverage.

## Verified shader roles

| SHA256 prefix | Bytes | Role identified from resource bindings/instructions |
| --- | ---: | --- |
| a9e5b544d5ec | 71144 | Direct lighting and selected spot/point ID production |
| 6af3f7403a4e | 50752 | Selected spot/point lighting with filtered RT visibility |
| feb37637481f | 4868 | Shadow temporal reprojection with depth and velocity |
| 584d40854439 | 12360 | Shadow filter using depth, light data, selected IDs and hit distances |
| e1a78e01b315 | 12360 | Second shadow filter variant with the same resource layout |
| c89bf47249ba | 7932 | Point-light cell construction from clipping data/BVH |
| acdf0bc33c0c | 10036 | Spot-light cell construction from clipping data/BVH |
| 9cb31a96edff | 3784 | Geometry skinning variant |
| 017bd2265499 | 3848 | Geometry skinning variant |
| e9105d227afa | 6516 | Geometry skinning variant |
| 32047fd063b5 | 87560 | Mod RR guide generation; not native shadow composition |

Being bound after RT enable did not establish a shader's role. The capture
included geometry and mod-guide shaders as well as native lighting shaders.
The decoder now handles SM5.0's 32-byte bindings and custom-data blocks, both
present in this capture. Previously a zero-length opcode for custom data could
fail to advance the instruction cursor. All 11 compute shaders now decode.

## Exact composition boundary

Offsets below are DWORD offsets within the SHEX instruction stream, not file
byte offsets. All references require the exact full shader SHA256, not a prefix.

In `a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f`,
`g_rwtSingleSpot` is UAV u2. DWORD 14809 stores the selected spot/point IDs.
The spotlight loop updates its selected candidate at DWORDs 10453–10524 and
copies the selected spotlight ID at 10565. Selection depends on evaluated light
contributions, so this path does not provide all-light RT coverage.

In `6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621`:

- `g_tSingleSpot` is t14; DWORD 354 loads the spot/point IDs. The subsequent
  comparisons reject the 65535 sentinel before evaluating either light.
- `g_tSingleSpotShadow` is t15; DWORD 346 loads filtered spot visibility into
  r7.z and point visibility into r7.w.
- Spotlight shadow-map inputs remain t10 (PCF) and t11 (VSM). DWORDs 1652–1707
  combine static/dynamic map visibility into r0.x.
- DWORD 1716 tests this map visibility, and 1723 skips the light when it is zero.
  A replacement must act before that gate. Changing only the final accumulation
  would not recover a light already rejected here.
- DWORDs 5986 and 6009 multiply the selected spotlight's evaluated diffuse and
  specular contributions by r7.z and add them to the existing light buffers.
- The point-light branch begins at 6025 and uses r7.w at 10850/10873.

Thus the native selected-light pass retains map shadowing AND RT contact
visibility. Extending contact rays alone cannot remove the map-filter artifact.
A first spotlight replacement can target this selected-light pass without
removing map shadows from other lights, but must coordinate visibility replacement
with a valid full-distance ray result. No such runtime patch is installed yet.

## Denoising and remaining implementation

The temporal shader reprojects with velocity, rejects invalid/depth-mismatched
history, and blends current/history visibility. The two additional filters read
light identities and hit distances. This verifies that native filtering exists;
it does not establish suitability for full-distance soft shadows or correct
history behavior when switching modes or selected lights.

The R64 native ray library limits spotlight distance to roughly 0.5–1.5 engine
units, and derives its radius multiplier from that limited distance. The
replacement must decouple ray reach from softness. Native alpha-test handlers,
shader-table identifiers, local roots and point-light behavior must be preserved.

Remaining runtime work: add a full-distance spotlight raygen to the same native
state object; select its own raygen record while preserving native hit/miss
records; coordinate its use with a guarded selected-light compute variant;
handle history when switching live; validate caster coverage, penumbrae and GPU
cost in game. The capture closes the missing-bytecode gap; it does not yet prove
a visual fix. No additional capture is requested on the basis of this analysis.
