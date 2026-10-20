# R53 native lighting isolation

Diagnostic based directly on R49, not a quality fix. R50/R51/R52 renderer
experiments are excluded. Existing skin, glass and native-menu behavior is
preserved. HBAO+ remains a separate roadmap item.

The native capture has strong noise by global event 543: deferred-light
compute 541 has written resource 2590, then illumination-volume draw 543
adds baked GI into it. Thus that exported buffer cannot attribute the noise
to either contribution. Event-418 normal/material input analysis found
constant GBuffer2 data on sampled carton/wall patches. A local linear fit of
normal and roughness detail explains little of the corresponding lighting
residual. This supports isolating lighting, but is not proof of its cause.

## Modes

Use the packaged selector beside Control_DX12.exe with the game closed:

- 0: native comparison, no R53 markers.
- 1: disable the exact captured illumination-volume shader's RGB contribution.
- 2: disable the exact captured deferred-light shader's diffuse/specular output.

Restart for every change. The package defaults to mode 1. If both R53 markers
exist, select native and log the conflict. R51/R52 markers have no effect.

These names describe two captured contributions, not exhaustive direct-only
or GI-only rendering modes. Other passes, forward lighting, emissives,
transparency, skin shading and postprocessing remain. Exposure can respond to
the altered scene, so darkness or brighter residual grain alone is not proof.
Unsupported shader variants pass through; start with RT/DLSS/RR/FG/glass OFF.
Do not assess RR compatibility using this intentionally incomplete lighting.

## Implementation and validation

Both patches retain instructions, resources, signatures and shader lengths.
Only selected immediate constants and the DXBC checksum change. Complete native
SHA-256 and instruction guards are checked before copying. Patched SHA-256 is
checked before PSO creation. A rejected replacement falls back to native.

Baked GI: 17,040-byte PSO3648 pixel shader, SHEX dword2093. Replace the 1/pi
factor feeding the final output with zero. Existing additive blend then adds
zero RGB. The shader's alpha is already zero and RGB is the only write mask.
This preserves its inputs, draw, and surrounding pipeline state.

Direct: 69,704-byte PSO3638 compute shader, SHEX dword14555. Set both dither
scale and bias to zero, making the two final multipliers zero for the diffuse
and specular UAV stores. This does not disable all other native lighting.

The existing early compute hook handles mode2. Mode1 adds a narrow graphics
PSO hook at device creation. The new hook forwards all nonmatching PSOs and
leaves another incompatible device-vtable original untouched. No extra GPU
passes, textures, sampling or image blur are introduced. Both modes retain the
original shader work, so this is not a performance optimization.

Local verification: portable C++ patch output is byte-identical to independent
patches; each disassembly differs by exactly one instruction; bindings and
constant metadata match; native and patched DXBC checksums verify; unknown,
truncated and instruction-mismatched inputs reject. The 132-file baseline audit
allows only the exact R53 hook integration. Windows CI builds separately.
User GPU confirmation remains required: inspect matched=1 applied=1 logs.

## Test and interpretation

Compare the same stationary stair wall/carton view in native, GI-disabled,
and direct-disabled modes. Send three labeled screenshots and compact logs.
No new PIX capture is required for this test.

Logs must show NATIVE_LIGHTING_R53 contribution=BakedGi or contribution=Direct
with matched=1 applied=1 for the corresponding mode. Absence is not a negative
visual result: it means isolation has not been established for that run.

If noise persists with GI disabled, replacing baked atlas resolution is not a
sufficient remedy for that remaining noise. If noise persists with direct
lighting disabled, investigate the GI/occlusion/surface inputs and remaining
passes. Disappearance identifies a contribution to examine, not permission
to permanently remove the lighting or a proven root-cause diagnosis.
