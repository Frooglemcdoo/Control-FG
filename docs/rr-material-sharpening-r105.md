# R105: restore 60% and investigate material sharpening

## Build scope

R104's 90% clamp increased noise without resolving the reported material sharpening. R105 restores the shared clamp policy to 60%. WORLD ALBEDO retains GI filtering, centered filtered spotlight shadows, and textured Standard-material albedo. SPEC FILTER is the rejected R103 experiment and still bypasses the clamp; do not use it as the baseline. No new spatial blur, roughness remapping, material change, or temporal filter is introduced in this build.

## Evidence and limits

The supplied 26-second video shows coarse, patchy detail across the dark elevator wall panels and patterned surfaces while the camera moves through the scene. It has no embedded mode/log evidence or matched RR-off reference, so it cannot independently prove a specific shader caused the appearance. The user's repeated RR on/off observations remain the evidence tying the objectionable appearance to RR. Video compression also limits measurements of fine noise.

R103 SPEC FILTER recorded 3,012 validated native filter invocations with no filter rejection or RR evaluation failure, but visibly added noise/boiling. That experiment simultaneously changed temporal accumulation, native spatial passes, and clamping. R104 changed the clamp exponent alone and also failed visually. Neither test supports adopting those changes.

R102's existing captured frame 13810 provides more useful guide evidence:

- Render 2560x1440, output 3840x2160, preset F, reset=0, RR result=1, projection valid.
- The previously audited painting and wall regions contain textured diffuse albedo. Missing world albedo alone was not the fix.
- All normal/roughness components in this frame are finite. Normal-length percentiles 1/50/99 are 0.999502/0.999766/0.999989, consistent with half-float normalized normals. This does not validate their coordinate-space or temporal alignment.
- Painting roughness percentiles 1/50/99: 0.2900/0.3450/0.4038. Wall region: 0.4312/0.6274/0.9136. They are not all zero/default; the native encoding still needs verification against the material shader.
- Specular hit distance is unbound. The evaluation code explicitly clears specular/reflection motion vectors. This is a known missing reflection-reprojection input, not proof that it causes stationary diffuse-texture sharpening.
- Two consecutive captured frames cannot establish behavior over the several-second stationary interval reported by the user.

ROIs are painting x=1160:1650,y=530:710 and wall x=800:1700,y=170:370. Values are from the existing R102 capture, not the newly supplied video.

## Research findings

NVIDIA's RR programming guide requires normalized shading normals, linear roughness and reflectance, coherent motion/depth/jitter, and describes specular motion vectors or hit distance plus matrices for reflection reprojection. It explicitly says RR ignores the ordinary DLSS sharpness option. The mod's Streamline setup already sets that option to zero; the active native NGX wrapper must also be audited rather than assuming that option controls all post-processing.

A concrete guide approximation exists in `src/shaders/rr_live_guides.hlsl`: the environment-BRDF lookup uses `abs(normalView.z)` as the view-angle coordinate. A perspective camera generally requires the angle between the shading normal and each pixel's view ray. Using the captured projection, the absolute discrepancy between those two angle estimates has median/95th-percentile values 0.0030/0.0065 on the painting and 0.0405/0.1098 in the wall ROI. Across the frame, they are 0.0835/0.2834. These are angle-coordinate differences, not measured specular-albedo errors. The calculation assumes conventional D3D screen-to-view orientation and omits subpixel jitter. Before changing it, verify the native LUT axis conventions and shader coordinate conventions. The small painting discrepancy means this approximation cannot yet explain all reported surfaces.

The native AA wrapper still forwards the game's f3 sharpening argument while RR is requested; the custom sharpening override is deliberately SR/DLAA-only. This is not evidence that an extra sharpen pass executes. Trace the native wrapper's consumers and compare captured RR output against the final presented frame before changing f3 or disabling any pass.

The inspected mod source does not show a deliberate RR-only global texture-mip bias override. That does not establish the native engine's sampler bias or normal-map LOD. Those need validation at the material producers, especially if detail grows after motion stops.

## Next investigation, in order

1. Trace stationary temporal convergence: correlate current-frame input/guide jitter, motion-vector units/sign, reset events, and material sampling with a short sequence spanning motion and several seconds of rest. Compare raw RR output with final presentation to locate whether detail is exaggerated inside RR or afterward. Reuse the existing input capture mechanism; do not add continuous render-thread I/O or GPU waits.
2. Audit material interpretation: verify native roughness encoding, F0 and BRDF-LUT conventions; then isolate a perspective-correct specular-albedo guide comparison, preserving skin/hair behavior and all successful lighting fixes. Do not smooth every normal or force all materials rough.
3. Recover valid same-frame reflection hit distance/motion from the native reflection producer. Do not substitute primary depth or fabricated distance. Check this separately from guide-angle changes so a visual result is attributable.
4. If those inputs are correct, compare native mip selection and post-RR sharpening, then assess remaining model behavior against a matched reference. Do not cycle clamp values or assume a successful evaluation means the integration is visually correct.

This is a research direction, not a claimed fix. The only rendering change in R105 is restoring 60%.

## Primary references

- NVIDIA Streamline RR programming guide, sections 4.1, 4.2.1, 5, 6 and 10: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md
- NVIDIA RR validation checklist, temporal settle, derived-buffer producer chains, motion-vector conventions and composition order: https://github.com/NVIDIA-RTX/Streamline/blob/main/plugins/streamline-agent-skills/skills/integrate-dlss-rr/references/validation-checklist.md

These are current upstream documents (the programming guide identifies version 2.14.16); the mod remains on its existing SDK/runtime. No SDK, model or runtime upgrade is included or assumed to fix this issue.
