# R194: same-frame native-map / RT visibility comparison

This retains R193's accepted reflection sequence. It does not change normal
lighting or claim to fix the Board Room light loss. Keep SPEC Native and launch
with Start-Compact-Distance-Test.cmd option 2 (hit distance OFF).

1. Face the affected Board Room area from the previous capture location.
2. Open F10 and click TRACE SHADOW. The button now explicitly requests MAP PROBE.
3. Hold still and keep the game focused until the completion dialog.
4. Send lighting.log, manifest.txt and all three PNGs from the Shadow-R194 folder.

The diagnostic temporarily disables RR and replaces diffuse RGB with encoded
visibility data. Strange colors are expected and are not a regression. It
restores your previous lighting mode when complete or cancelled. F9 cancels an
active light trace before it can start the separate reflection suite. The full
F9 reflection capture is not needed. No exact cross-run camera alignment is
required because the numeric comparison uses the same captured frame/pixels.

The already-implemented selected-map probe records native shadow-map visibility,
its complement and selected spotlight ID before composition. Existing capture
records provide filtered RT visibility and full-length reference hit geometry
for those grid pixels. The analyzer checks ID, payload complement, GPU fences,
frame provenance and complete export before comparing. IDs >=64 are rejected
because the diagnostic blue lane cannot encode them reliably. The dominant
light in the previous capture was ID22; IDs remain capture-local.

A native-lit / RT-blocked result establishes a visibility disagreement. It does
not by itself distinguish material alpha, shadow-caster inclusion, map bias,
projection or filtering. Agreement does not prove physical correctness either.
The output is diagnostic, not radiance, so do not judge normal lighting from
its PNG colors. Original game alpha testing remains in the reference hit
pipeline; no blanket transparency bypass is added.

Rendering defaults have not been changed in this instrumentation build. The
old launcher label still says R190; option 2 affects only the compact guide.
Some reflection-library/probe log lines retain R193 labels because those paths
are unchanged. The main diagnostic log and shadow-capture folders say R194.
