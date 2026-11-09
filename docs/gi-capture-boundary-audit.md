# GI producer capture: verified boundary and remaining implementation

The existing RR capture cannot be extended by simply appending the three GI hit textures at the filter boundary. A new static audit verifies this against the supplied renderer and all eight native GI shading variants. This is a capture implementation finding, not a speckle root-cause claim.

## Proven in the supplied binaries

- GI hit resources are bound for shading at renderer RVA `0x12c61c` through `0xa3940`.
- The GI shader is bound at `0x12cbd2`; the compute dispatch call is at `0x12cc16`.
- After that dispatch, a batch counter increments and can loop back to `0x12bfa6`. A capture must distinguish each dispatch, not just frames.
- After the batch loop, `0x12cc4f` calls the container cleanup function `0xa3190`. That function zeros the material, normal and position container pointers and passes the previous pointers to its cleanup helper. This does not prove the ShaderTexture globals are zeroed or establish the later GPU allocation lifetime; it means a later read cannot assume original ownership and content remain valid.
- The GI filter call is later, at `0x12cd96`. Our existing early-GI image capture occurs within this filter. It is appropriate for the GI image but is not a proven hit-input capture boundary.
- All eight GI shading variants declare material, normal/UV and position/UV as `Texture2DArray`. The per-ray loop uses `g_uRTDiffuseRayCount` at byte 8 of CB4, with the loop counter as the array coordinate. All three hit loads use the same coordinate; the counter increments by one.
- The actual SRV registers vary with the variant. Resolve bindings by the shader's verified reflection data, not a shared hard-coded slot table.

The audit checks exact renderer identity, instruction bytes/call targets, provider object addresses, shader identities, reflection layouts and executable load/loop operands. Run:

```text
python tools/audit-gi-capture-contract.py <renderer.dll> --output <report.json>
```

Committed output: `research/gi-source-r267/capture-contract.json`.

## Required implementation

Capture at the GI shading dispatch, after native dispatch setup has established readable inputs and before the batch back edge/cleanup. Revalidate the engine command context and resource states there. Copy every used slice for every batch into bounded ROI readbacks, retaining resource ownership through the actual submission fence. The source SRV's first slice, array size and mip selection must be recorded; a resource description alone is insufficient. Fail explicitly on capacity overflow, missing batches/slices, layout changes or incomplete GPU work.

Capture the current constants and the spotlight records/cluster indices at this same boundary. The hit textures alone cannot reveal the per-light attenuation, cookie, visibility or radiance intermediates; those require instrumenting the verified native shading instructions or a verified replay with the complete input set. Preserve the native formula, shadow visibility, projection cookie and material response during measurement.

Correlate that record with the existing pre-filter GI image, RR input and RR output by frame and command-list generation. Only then attribute bright samples to incorrect inputs/weights versus valid high-variance illumination.

## Status

The capture contract audit is implemented and passes for the supplied renderer and all eight GI variants. Runtime per-batch GPU capture is **not implemented** by this commit. No new test build is claimed.

The local runtime default is restored from R270's white-cookie diagnostic to full native GI lighting. Explicit diagnostic launch options still work. This source change has not yet been packaged/deployed to the user's game. The accepted R264 ray sequence and existing lighting/filter settings are untouched.

R271 follow-up: bounded physical hit-array capture is now implemented at the audited dispatch, with contiguous batch validation and collection alongside RR image pairs. See `docs/native-lighting-r271.md`. Spotlight records, per-light operands and verified SRV-to-ray mapping remain outstanding; the complete producer trace described above is not claimed.
