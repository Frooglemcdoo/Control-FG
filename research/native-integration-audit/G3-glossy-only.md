# G3: remove added diffuse, retain glossy

User explicitly rejected added diffuse after G2D5: performance regressed, diffuse flickered, and image benefit was negligible. Restored the proven G1R4 one-layer glossy raygen, resource pool, lifetime/handoff implementation, and full native lighting translation. Preserved a two-state Native/Glossy UI; original opaque runtime hash gates remain unchanged apart from exact authorization for these UI strings. Removed diffuse/history/diagnostic shaders, buffers, UAV bindings, readbacks, and obsolete tests from the active codebase. Native diffuse lighting is not removed.

The secondary lighting texture now explicitly writes alpha 0 for invalid/unshaded results and alpha 1 for completed shading. The glossy compositor requires alpha > 0.5 and finite RGB before replacing native indirect specular. This prevents missing evaluation from substituting black, while legitimate shaded black remains valid. This is a defensive validity fix, not evidence that this was the cause of the reported scene difference.

## Remaining color/information audit

Glass replaces r11.xyz at dword 6737. Native r11 comes from GI directional bricks with normalization and ambient addition (dwords 2138, 2194, 2210, 2228, 2253), or environment cubemap fallback (2599). The traced path replaces that entire directional specular source for accepted glossy hits. Direct lights, native diffuse and emission remain in the native calculation. One ideal glossy ray cannot fully reproduce a prefiltered environment lobe, especially at the current smoothness threshold of 0.8.

Opaque PT independently computes a specular substitution delta (replacement-reference), with a native parity check, in pt_reflection_p2_native_composite.hlsl. That path is not changed here because the supplied glass-flicker video does not provide a fixed-view native/PT color comparison that isolates it. No arbitrary brightness floor, RGB maximum, tint boost, or gain is introduced.

Next scene evidence: camera-fixed PT OFF, PT ON + GLASS NATIVE, PT ON + GLASS GLOSSY. The first comparison isolates opaque PT; the second isolates glass continuation. Keep other rendering settings unchanged. Missing detail should be localized before changing either substitution model.
