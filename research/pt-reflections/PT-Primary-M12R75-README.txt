M12R75 - split PT signals and variance-guided reconstruction

This is the first reconstruction implementation from the R74 research, not a
claim that every reported defect is fixed. The current primary material/depth
mismatch remains strictly rejected. New independent counters distinguish
same-depth material disagreement from wrong-depth geometry. Previously the
ordered checks hid this distinction.

The existing path sample is decomposed into diffuse and specular contributions
using the evaluated primary BSDF fractions. Sampled-lobe selection is NOT used
to label a mixture-BSDF estimate. No extra rays or light intensity gain.

PT writes diffuse/specular, reversible material factors, roughness and first
continuation hit distance. Reconstruction demodulates the signals, accumulates
separate channels and luminance second moments, and filters at pixel spacings
1 and 2. Material/normal/plane boundaries remain gated; specular filtering also
uses roughness and hit distance. Recombination restores material factors before
native primary emission/fog. Temporal feedback uses the unblurred channel history.
This is a custom variance-guided filter, NOT an NRD integration.

Raw output increases from 16 to 64 bytes/pixel. Two pooled 32-byte/pixel buffers
hold channel history and filter scratch. Final native-consumer layout remains
32 bytes/pixel. Resource reuse requires completed GPU work and released owners.
The auxiliary pool is capped at 2 GiB, separately from the existing 2 GiB output
pool. This increases memory/bandwidth; it is not an FPS optimization. All buffers
use the admitted internal render dimensions, not the display resolution.

Known unfinished work: primary surface correspondence, object-motion vectors,
explicit emissive-light sampling, environment intensity/source audit and
unsupported secondary materials. Camera-only history cannot correctly track
moving objects. RR remains unchanged. No claim that missing lights are restored.

Validation: CPU transport checks plus Windows shader/build and WARP tests.
The split-buffer GPU test uses changing albedo, independent diffuse/specular
energy, history renumbering, disocclusion, and rejected/probe admission. Actual
game quality, GPU cost and memory pressure still require the game run.

Close Control and replace ALL package files. Keep existing game/PT settings.
Do not rerun PT-Enable when already enabled. Collect-PT-Primary-M12R75.cmd
collects the new independent correspondence categories as part of the full log.
