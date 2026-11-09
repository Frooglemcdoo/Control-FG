# R268: GI spotlight PCF comparison test

R267 user result: removing point-light GI does not improve speckles; removing
spot-light GI clears visible hair speckles and most plant speckles. This build
restores full spotlight GI and tests a specific shadow-sampling correction.
It is not yet a visually confirmed fix.

The native GI PCF branch samples a depth atlas through `g_sLinearClamp`, then
compares that filtered depth with receiver depth. R268 compares four texels
individually, then bilinearly weights their 0/1 visibility. That matches the
comparison-filtering order described by Microsoft:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-to-samplecmp

This is a separate path from the primary RT spotlight fixes. R71 corrected
traversal/blocker-distance and filter inputs; R94 corrected light selection.
Neither is directly applicable to the native GI shadow-map sample. R268 does
not claim their exact defects have been found here. The lamp's active PCF/VSM
branch is still unverified; if it uses VSM, this change may have no effect.

## Scope

All eight hash-verified native GI shaders receive the same replacement in their
PCF branch. Point lighting, VSM shadows, cookies, distance falloff, material
response, light blending, ray counts, GI history policy and RR remain unchanged.
The original shadow comparison direction and receiver depth are retained.
Four taps are clamped within each light's atlas rectangle to avoid borrowing
depth from an adjacent light. No material, light-ID or room exceptions exist.

This adds three texture fetches, one dimension query and arithmetic per executed
GI spotlight PCF sample, plus seven temporary vector registers. It adds no rays,
passes, readbacks or temporal history. GPU cost and visual quality are unmeasured.
The change affects GI whenever the recognized shader is used, including RR OFF;
this is a diagnostic build, not a new production default.

## Test

Install the complete package beside Control_DX12.exe with folders intact.
Launch normally; no source-removal launcher is needed. Keep the lamp intact,
RT indirect lighting ON, RR ON, Native Blend, GI RAW BYPASS, the same GI sample
count as R267, and FG OFF. After gameplay, F10 should say GI SHADOW: PCF TEST.
If it says no shader bound, collect logs and do not treat that run as applied.

Check the hair and plant stationary and moving, also shadow edges and brightness.
Full spotlight bounced illumination should return. If needed, use
`ControlTOH/Diagnostics/Start-GI-Shadow-Test.cmd` option 2 for native sampling;
option 1 enables the correction. Restart between comparisons. Both retain full
spotlight GI. The old source probe is independent and disables this PCF change
while a contribution-removal option is selected.

## Validation

Generator checks exact native identities, unchanged reflected bindings and
constant buffers, and inverse reconstruction of the original shader container.
Portable tests interpret the emitted instructions against an independent PCF
reference for 1,600 random, boundary, one-texel, lit and shadowed cases across
eight variants, plus continuous edge-ramp cases. Windows validation disassembles
and creates WARP PSOs from every full replacement shader. This does not execute
the complete game scene or prove that the lamp takes the modified branch.
