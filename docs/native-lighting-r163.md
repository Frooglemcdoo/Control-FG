# R163: selected spotlight map visibility probe

R162's projection-origin comparison was bound at ray record offset 448 with RR bypassed. The user's screenshot still shows the incorrect diagonal floor boundary. Changing the source position did not resolve it. CPU binding logs do not independently confirm GPU execution; the UI selected the expected mode and no selection failure was reported.

R163 adds MAP PROBE, a diagnostic measurement rather than a rendering fix. It retains the accepted light ranking, selected IDs, authored rays, geometric normals, centered filtering and current-frame spotlight policy. A private shader register follows the exact existing winning-candidate condition and retains that candidate's native static/dynamic map visibility. After native output dithering, the producer writes (map visibility, its complement, selected spotlight ID + 1) into diffuse RGB. This deliberately replaces diffuse radiance in this mode only; unusual colors and brightness are expected.

TRACE SHADOW preserves MAP PROBE when selected, otherwise using the accepted RR OFF reference mode. The existing composition-before sampler captures the encoded map value alongside the same pixel/light's filtered RT visibility. Independent ray/composition provenance, asynchronous export, five-second settling and three screenshots remain. No new resources, GPU waits, additional rays, per-frame readbacks or render-thread file I/O are introduced.

Default rendering and existing shader assets remain unchanged. MAP PRIORITY remains rejected. The failed projection-origin test remains internal, but MAP PROBE replaces its overlay shortcut.

## Test

1. Close Control, install beside Control_DX12.exe, and start a fresh session.
2. Stand on the stairs where the incorrect diagonal floor shadow is visible.
3. Open F10, choose MAP PROBE, then click TRACE SHADOW.
4. Hold the same view through the five-second settle and capture. Diagnostic colors are expected. Do not move into the room.
5. After completion, choose DEFAULT (FIXED).
6. Send the Shadow-R163 folder containing lighting.log, manifest.txt and the three PNGs. Use Package-Lighting-Capture.cmd if needed; it splits below 200 MB.

No large PIX capture or additional room traversal is needed.

## Interpretation and limits

Use tools/analyze-map-visibility-probe.py on lighting.log. It requires exactly one MAP PROBE trace and the existing complete fenced capture. It checks finite values, map/complement consistency and the encoded ID against the independent selected-light record. Because the payload may pass through R11G11B10, only IDs 0–63 are accepted; larger IDs are explicitly rejected, not rounded into identities. Map values retain render-target quantization, with complement tolerance 0.012. An intervening writer or unexpected format should produce a rejected payload; these checks cannot detect every possible coincidental modification.

Compare map and RT visibility numerically, not the diagnostic screenshot's final brightness. Diffuse-before/after/delta are not radiance in this mode. Native map visibility includes its native bias/filtering; RT visibility is the accepted spatially filtered result. A disagreement identifies a visibility mismatch but does not alone distinguish caster inclusion, bias, filtering or authored geometry intent. The auxiliary hit ray remains the existing full-length reference, not the four-ray gameplay result. No claim that the false shadow is fixed.
