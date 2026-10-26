# R174 — skip discarded area-light roughness arithmetic

R172 remains the user-reported 5–7 FPS gain baseline. R173 gave identical performance; its Save Log reports zero observed pipeline-library loads. That log did not contain the replacement creation counters, so direct replacement activation was not established by it.

## Rendering change

The native reflection resolve computes a 40-instruction area-light roughness adjustment for each point light and each applicable spot light, then uses movc to select either the result or unchanged material roughness. When the existing nonzero radius/distance-product condition is false, that arithmetic result is discarded.

R174 wraps each block in an IF using that exact existing condition. The original movc remains after the block. Nonzero/NaN conditions retain the original arithmetic; a zero product skips only the discarded calculation. This removes no ray sample, light, shadow-map tap, material texture or accumulated contribution. All other instruction bytes are retained, including R172's two-load movement. No register declaration or thread-group change is introduced.

The source shader remains hash-locked to 4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9. The candidate SHA256 is ca3b09acddec81ba7a208266b0500520c24433beb3d7bd3add51723e95a707f6. Existing native fallback remains available if PSO creation fails.

## Validation

A conservative full control-flow liveness audit checks loop backedges and branch exits. For each block, its only written component live at the native selector is the discarded result; the condition itself is unchanged. The blocks contain pure arithmetic only. Removing the four added IF/ENDIF instructions recovers R172 exactly.

Windows WARP fixtures execute the actual native 40-instruction blocks and their guarded counterparts with alternating zero/nonzero conditions for both light types. They retain the native sample termination, clamp and final reduction. The test uses the same serial dispatch for both shaders across sample counts 0/1/2/4/8/12/16, clamps enabled/disabled, partial groups and sentinel lanes. This is a synthetic arithmetic/equality test, not proof of full-scene equivalence or speed.

The existing serial-runtime helper tests and Windows full-shader PSO validation also run. No measured speedup is claimed: scene light radii, compiler optimization and branch divergence determine whether this helps.

## Capture and comparison

Save Log now includes direct/library replacement creation counters and failures. Exported filenames identify R174 correctly. Creation is not GPU-execution proof, but zero successful creations means this candidate has not been confirmed applied.

Restart after installation. Compare the same view/route against R172/R173 with SPEC 16, GI 6, NATIVE BLEND, RR on, FG off. Check reflection detail and lighting during movement, then use PERF CAPTURE and Save Log. All 16 samples and accepted GI, shadow, skin/hair settings remain unchanged. Retain only if measurements improve without visual regression.
