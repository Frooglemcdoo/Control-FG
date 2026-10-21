# Native spotlight extraction adapter

This is an adapter component, not an installable shadow replacement. The live
R79 renderer is unchanged. Test 1's DXR backend passed on the user's RTX 5090.
Native extraction and contribution packing now also execute successfully on
Windows WARP; this validates the GPU programs using controlled native-format
inputs, not live Control scene bindings or visual quality.

## Data flow

The adapter derives a compute shader from the captured native composition shader
`6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621`.
`tools/rt_lights_native_extract.py` rejects every other source hash. It retains
the native spotlight material, falloff, projection, cone and translucency code.

The t14 selection texture becomes an R32G32B32A32_UINT work atlas. Each texel
contains `(lightIndex, receiverX, receiverY, enabled=1)`. Two atlas texels can
reference different lights at the same receiver, or entirely different receivers.
Both native uses of dispatch coordinates for screen input fetches are redirected
to the receiver coordinates. Output stores still address the work atlas.

The adapter:

- Rejects disabled work, the 65535 sentinel, light indices outside the bound
  structured view and receiver pixels outside the GBuffer.
- Replaces combined static/dynamic map visibility before its zero-energy gate.
  The preceding map calculations remain for now to preserve their other live
  intermediate values. This is not yet the optimized production variant.
- Removes the old selected-light visibility input and both additive UAV reads.
- Writes separate diffuse/specular RGB, with alpha 1 for valid extraction.
  Invalid work is overwritten with zero and alpha 0; valid but unlit work has
  zero RGB and alpha 1.
- Ends before the native point-light branch. Point lights are not yet converted.

`rt_lights_native_pack.hlsl` converts the two output atlases into the shared
`Contribution` buffer. It checks dimensions, strides, enabled work, pixel bounds,
finite lighting, extraction validity, light/receiver identity and frame tag.
Invalid entries use visibilityIndex UINT_MAX so composition rejects the receiver
instead of silently treating missing data as a shadowed or absent light.

## Binding and lifetime requirements

The game adapter must provide native-compatible views for all declared bindings,
including the map inputs retained by the extraction shader. Material indices and
native material resource extents must be valid together. All GBuffer inputs must
refer to the same resolution, eye and frame. Use separate atlas outputs; never
bind the game's accumulated lighting buffers as extraction outputs.

Dispatch extraction in 8x8 groups with depth 1. Pad unused work texels with zero.
The work atlas and both output atlases must have matching dimensions. Preserve
native sys_constants and sampler conventions. The extraction shader uses native
space-1 samplers s0, s6 and s11.

After extraction, transition outputs from UAV to non-pixel SRV before packing.
Bind packing t0=work, t1=diffuse, t2=specular, t3=ShadowTask and u0=Contribution.
Its b0 is `(receiverWidth, receiverHeight, frameTag, workCount)`. Slot i must match
ShadowTask i and Visibility i. Packed receiver IDs are y*receiverWidth+x.
Keep the entire frame batch immutable until its consuming queue fence completes.
No CPU readback or wait belongs on the game render thread. Blocking helpers in
`tools/gpu_test_device.h` are exclusively for standalone fixtures.

## Verification and remaining work

The Windows fixture compiles with /W4 /WX and executes the generated shader and
packing shader. It checks nonzero red and blue lights at the same receiver,
separate diffuse/specular results, invalid indices/coordinates, disabled work,
the native sentinel and stale frame rejection. It also checks receiver coordinate
remapping and blocked map/contact inputs. The initial generated shader had invalid
UAV destination selectors; Windows validation caught this before game integration.
The generator now emits UAV write masks and scalar branch operands correctly.

Still required before an in-game test: complete native spotlight enumeration and
culling, native receiver/emitter ShadowTask production, verified native TLAS and
alpha SBT bindings, fence-retired frame resources, per-light filtering, and atomic
replacement of the corresponding native contributions. The old selected-light
filter/history cannot serve as the new all-light filter. No new installation or
manual fixture run is requested until these pieces support a useful game test.
