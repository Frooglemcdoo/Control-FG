# R196: native shadow-map capture alongside Native Blend

Keep the R195 baseline: Native Blend, RR ON, SPEC Native, compact hit distance
OFF and the accepted game RT settings. The accepted reflection fixes remain.
This is a diagnostic build, not a claimed fix for the Board Room lighting.

## Capture

At the affected location, open F10 and choose TRACE SHADOW. Hold still until
completion. Send lighting.log, manifest.txt and the three PNGs in Shadow-R196.
No MAP PROBE mode, RR-off phase or alternate lighting image is required.

For the captured frame only, an instrumented version of the exact corrected
light selector retains every native instruction and output write and adds the
winning light's native shadow-map visibility to a private float32 texture.
The paired composition capture samples this texture alongside filtered RT
visibility and light IDs. Ordinary diffuse/specular outputs remain radiance.
The previous R11G11B10 diagnostic payload's light-ID limit does not apply.

Readback checks frame identity, command-list generations, submission ordering
on the same queue and GPU fences. Missing selector data, reordered/different
queue submission, replay or reset-before-submit fails the capture explicitly.
The private texture is bounded to 16,777,216 pixels (256 MiB), retained safely
through GPU completion, and reused for later captures at the same dimensions.
It is allocated only when the requested capture reaches the selector pass.
Capture adds GPU work and memory; do not measure gameplay performance during it.

Use tools/analyze-map-visibility-probe.py on lighting.log. R196 Native Blend
captures require the private map payload; ordinary R195 captures are rejected.
The old MAP PROBE decoder remains compatible with earlier diagnostic captures.

The comparison uses the selected light in the accepted corrected selector,
not a second replay of the game's unmodified light-ranking algorithm. The map
uses native bias/filtering; RT uses the accepted filtered shadow visibility.
Disagreement alone does not prove whether caster selection, alpha/material
handling, bias or filtering is responsible. The 4096 reference rays remain
full-length reference segments, not every gameplay shadow ray.

Validation includes exact selector-instruction preservation, private payload
and provenance rejection, Windows shader creation, GPU patterned-texture
readback, and existing baseline/capture tests. In-game validation is still
required. Production/main/releases are unchanged.
