# GI29 — Direct Six-Shader DLF Parity

GI29 changes the test strategy. Instead of approximating RenoDX behavior by changing Control's filter-function arguments, it replaces the exact six DeferredLightFiltering compute PSOs by their native shader CRC:

- `0x600347E7` specular temporal pre-accumulator
- `0x591FC46F` diffuse temporal pre-accumulator
- `0xBA41374D` specular spatial X
- `0x87EDDD47` specular spatial Y
- `0x9018E4F2` diffuse spatial X
- `0x2A6F7863` diffuse spatial Y

The replacement PSOs keep Control's native root signature and descriptors. GI29 direct mode makes Control's three reviewed RR render-option decisions report the normal OFF state so the normal DLF dispatch structure executes and the replacement shaders are what remove history/spatial filtering.

Important user constraint: the existing specular clamp stays **0.60**. GI29 uses the already-compiled 60% specular temporal replacement, not RenoDX's 1.0 specular clamp. The diffuse temporal clamp stays at RenoDX's 1.0. Both temporal shaders have history weight zero. All four spatial replacements are passthrough.

In direct mode, Control's separate contact-shadow filter is left untouched.

Runtime proof requires all six target PSOs to be captured before RR can become ready. Each filter call also verifies that the expected replacement PSO was actually selected:
- `RR_GI29_DLF_CAPTURE ... mask=0x3F`
- `RR_GI29_DLF_SET ...`
- `RR_GI29_DLF_SPECULAR ... temporal_used=1 spatial_used=1`
- `RR_GI29_DLF_DIFFUSE ... temporal_used=1 spatial_used=1`
- `RR_GI29_CONTACT_SHADOW ... mode=native_untouched`

F10 -> RR Settings -> **Direct RenoDX DLF shaders** toggles the direct path. It defaults ON.
