# R145 — composition signal capture

Diagnostic build; accepted R142 rendering policy and R134/R139 shader fixes stay
unchanged. This adds measurement, not a confirmed fix for disappearing shadows.

## Test

Install with the game closed. Use Default, RT Contact Shadows on, samples 1,
FG and glass enhancements off. Press Shift+F9, walk through the affected room
and back, and wait for completion (typically 25–40 seconds). No mode cycling.
Send lighting.log and manifest.txt from Shadow-R145. A video is useful if available.
F9/Shift+F9 cancels; loss of focus cancels. Plain F9 remains the stationary suite.

For large captures, drag the completed folder onto Package-Lighting-Capture.cmd.
It uses installed Python 3.8+ without additional packages. Send **all** ZIPs in the
new SEND folder, including capture-manifest.zip. Each is below 200,000,000 bytes;
originals remain intact. A single oversized file is split and can be reassembled.

## Measurements

For each of the same six 64×64 ray/context samples, read the native composition
input selected spot/point IDs and filtered visibility, plus the completed native
diffuse and specular UAV outputs. These values precede downstream material/post
processing and tone mapping; they are **not** final pixels or an exposure reading.
Native output includes its existing accumulated lighting; it is not an isolated
per-light radiance measurement. Group averages cannot establish causality because
moving the camera changes sampled surfaces. Compare corresponding surfaces.

The new compute sampler runs after the identified composition dispatch, in the
same verified spotlight scope and command-list generation as the ray capture.
It reads native textures without writing them. Private descriptors replace only
the diagnostic constants and output buffer. Native root table and selected PSO
are restored. A UAV barrier orders composition writes before readback sampling.
The existing queue completion fence covers both capture stages. No render-thread
fence waits, repeated allocation, frame-based resource retirement or idle sampling.
A missing composition stage fails capture rather than exporting stale signals.

Record extension: `signal_bytes=64`; per-record `signal=` contains 4 uints
(pixel x/y, spot/point IDs), then 3 float4s (visibility, diffuse, specular).
Decoder requires matching ray pixel and selected spotlight, finite values, and
spot visibility in [0,1] (with rounding tolerance). Legacy R133/R144 logs remain
readable. HDR lighting values are preserved without clamping.

## R144 evidence

Uploaded lighting.log and manifest.txt completed all six samples with zero
failures: 24,576 valid rays, 2560×1440, unchanged Default mode sequence.
The principal emitter at [47.067,33.146,-55.188] remains selected in all six
samples; indices are 16,16,15,15,16,16. Another emitter at
[67.377,34.836,-77.890] has indices 45,42,41,42,45,43. These are geometric
matches, not engine UUIDs. Color changes in an index debug view do not prove a
physical light changed. Sample counts vary with camera position.

Exploratory mutual-nearest receiver matching within 0.03 engine units found
10 different-light matches among 666 nearby pairs. These are nearby positions,
not proven identical receivers; selection boundaries/material response can
explain some changes. This does not establish why displayed shadows disappear.
The next evidence needed is the actual composition shadow signal and lighting
values at the affected surfaces, now recorded by this build.

## Validation

Python decoder tests cover old ABI compatibility, HDR values, pixel/light
provenance, missing signals and nonfinite values. WARP exercises the real sampler
on 4096 patterned pixels, including integer IDs and HDR output, and rejects
submission before the composition sample exists. In-game validation is pending.
