# Entrance full-PT replacement boundary

Evidence: six native, proxy-disabled captures from Entrance-20261012-100912-390040,
E3 AllSource export and ShaderBlocks prefixes. Names 05-rr-off and
06-composite-off are pose labels, not verified mod comparisons.

## Confirmed boundary

The current `pt_reflection_p2_native_composite.hlsl` preserves native diffuse,
direct lighting and emission, and applies only a specular substitution delta.
Its `PTFirstHitThroughput` is specular-only. Its parity admission check can reject
the substitution. Thus this code is a hybrid reflection extension, not a full
primary-surface path tracer. No claim about its per-pixel acceptance rate can be
made from these native-only captures.

All six frames contain native deferred reflection shader SHA256
`4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9`.
This exactly matches the native evaluator already integrated into the mod.
The older native-deferred-light-abi.md description of an environment/emission-only
P2 implementation is historical and does not describe the current source.

## Captured pass identities

IDs below apply only to overview; the JSON maps every view by shader hash.
Roles are inferred from declared resources and current integration, not a claim
that every instruction or resource value has been validated.

| Overview PSO | Identified boundary | Declared outputs / relevant inputs |
|---|---|---|
| 6328 | Deferred reflected-hit shading | ReflectionTarget; material ID, normal/UV, position/UV; point/spot lights, shadow atlases, GI bricks, environment |
| 6336 | Additional hit-radiance shading | Radiance; material/hit buffers, light cells, shadows, GI and environment |
| 6341, 6342 | Lighting filtering | ColorTarget; color source/history, velocity, depth, hit info |
| 6347 | Reflection/shadow history processing | Reflections, NewShadow, BentNormals; old/new reflections and shadows, velocity/depth |
| 6340 | Primary deferred direct lighting | LightBufferDiffuse, LightBufferSpecular, SingleSpot; point/spot/sun light buffers, shadow data, G-buffers |

Native DXR hit shaders write hit attributes used by later compute shading.
Four DXR library hashes are identical across the six views. Recursion limits
in native state objects do not establish the number of lighting bounces.

## Implementation contract

1. Reuse verified native geometry, material indexing, alpha testing and light
   layouts. Separate material evaluation from native cached GI and atlas-based
   visibility before using it in a new integrator.
2. Add a radiance/throughput path state with diffuse and specular sampling,
   explicit PDFs and termination. A larger native recursion limit alone does
   not implement path tracing.
3. Evaluate sampled direct lights with traced visibility at each supported hit.
   Reusing native shadow atlases is a compatibility mode, not the final visibility
   model. Reflection light-cell coverage for arbitrary bounce positions must be
   established before treating those lists as complete light sampling data.
4. Provide separate primary diffuse/specular radiance outputs. Replace the
   corresponding native contributions rather than adding PT radiance over native
   direct/GI. Trace descriptor/resource ownership through final composition before
   disabling a pass: the primary pass also writes SingleSpot.
5. Define denoiser inputs, hit distances, motion, exposure and history reset rules
   for the new signals. Native filtering bindings are useful evidence, not proof
   that the existing filters or RR accept the replacement unchanged.
6. Preserve glass compatibility while the opaque entrance path is established.
   Full transmission and transparent-layer accounting remain separate work.

## Reproduction and validation

Run with Python and `dissect.util` installed:

```text
python research/pt-reflections/tools/map_entrance_pipelines.py ALLSOURCE.zip SHADERBLOCKS.zip inventory.json
```

The tool walks the exported PSO creation order, decompresses XPRESS blocks,
checks container bounds and decoded lengths, hashes individual shader containers,
reads legacy RDEF bindings and maps dispatches to PSO/state-object IDs. DXIL RDEF
bindings are explicitly null; those libraries require their separate DXIL dump.

Observed totals: 1,018 pipeline entries across six views, 471 dispatch calls.
Every dispatch's pipeline reference resolves. Each frame contains four DXR state
objects. These are offline structural checks, not a rendering or performance test.
Command source ordering is not asserted to be GPU execution ordering.

Remaining evidence work uses the existing exports: resolve actual descriptor
resources and final composition ownership, inspect primary lighting/GI consumers,
and establish the visibility/material ABI for arbitrary secondary hits. Raw
resource contents remain in the original captures/resources.bin on the user's PC.
No new capture or runtime change is required by this mapping step.
