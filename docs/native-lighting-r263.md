# R263: stationary GI-to-RR sequence

R262 user result: speckles return when the camera stops. All 57 recorded bypass
samples in the supplied run showed copy_applied=1, spatial_observed=0, complete=1.
RR evaluations succeeded. These sampled logs do not identify the pixel-level
origin of the speckles. Rendering, shaders and default settings are unchanged.

## Capture

Start a fresh game session. Use Native Blend, RR ON, FG OFF. Face the projector
floor where the speckles appear. Select **GI: RAW BYPASS**, then **CAPTURE RR**
once. The panel closes. Hold the camera still while four pairs save automatically.
Each pair contains two consecutive evaluated frames; the four pairs have settling,
allocation and export gaps between them. This is not eight consecutive frames.
Allow roughly a minute; export speed affects duration. Reopen F10 to check status
if needed (menu interruptions restart the settling timer). When **Saved pairs: 4**
shows complete, click **PACKAGE RR** and send every generated ZIP. If it fails,
PACKAGE RR retains partial captures and diagnostic information. Do not repeat
clicks or change modes during capture. No external video recording is needed.

## Signals and provenance

The new `pre-filter-gi.rgba16f` copy records the temporal shader's t3 source
immediately after the R262 pixel-copy dispatch commits native bindings. That
shader does not modify its input. This is before the enclosing native filter's
output copies and subsequent lighting composition. The source ShaderTexture
at renderer RVA 0x128f3c8 is identified by the native g_DLF_rwtColorSource binding
at 0x1553d; its actual resource and tracked read state must be verified.

`gi-at-rr-input.rgba16f` replaces the misleading raw-gi filename for this mode.
It records the GI texture at the RR evaluation boundary. Both copies are tied
to the same engine frame, with resource identities and acquisition timestamps.
The latter must match the observed filter color resource. Also captured:
composed RR input color, RR output, diffuse/specular accumulation, actual RR
normal/roughness, diffuse and specular guides, original GBuffers, depth, motion,
raw albedo when available, responsivity, projection, jitter, motion scale,
exposure metadata with validity flags, reset state and RR evaluation result.
No missing exposure value is invented.

These are snapshots of the actual resources supplied at the evaluation boundary;
correct contents/units/reprojection are still to be assessed from the capture.
The sequence does not automatically prove the camera was stationary. Motion,
depth, jitter and image differences will be checked offline.

## Resource lifetime and safety

Only two readback sets are allocated at once; four pairs reuse those slots after
GPU completion and export. Preparation/export run on workers. Copy recording
uses the existing native state/recording locks, verifies context, queue, device,
frame, extent and read state, then restores the exact state. No guessed writable
state is admitted. No readback is mapped before the native queue fence completes.
First-frame export waits until the second frame has been recorded. An early GI
copy that cannot be joined to the same RR frame fails the capture and retains
resources rather than overwriting/freeing in-flight data. A 120-second per-pair
watchdog ends stalled requests. Diagnostic failure does not alter rendering.

At 2560x1440, full-resource capture is sizable; eight frames may produce several
GB before compression. The package helper splits the archive as before. Capture
copy/export overhead makes this unsuitable for measuring ordinary frame pacing.

## Validation

Collector tests reject stale/missing early GI, non-bypass modes, spatial work,
wrong GI identities, missing projection/motion metadata, reset RR frames, broken
consecutive pairs, incomplete files, and configuration changes. UI tests cover
one-click four-pair sequencing, duplicate rejection, export gating and failures.
A WARP test runs the production early-copy function, native-state guard and GPU
copy recorder with real source/readback resources; it checks exact mip0 pixels,
two consecutive slots, stale/context/writable-state/lock rejection, duplicate
prevention and native state restoration. The live native provider and insertion
point still require confirmation by this in-game capture.
