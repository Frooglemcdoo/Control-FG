# R64: RT spotlight prototype and integration capture

Status: the standalone DXR library is compiled, but is **not connected to the
game's lighting pass**. This package leaves native shadows active. It is an
integration capture, not a visual fix or a complete RT shadow replacement.

## Why capture is necessary

The existing reflection library includes shadowClosestHit, shadowMiss and
shadowAlphaTestAnyHit. The closest-hit/miss handlers write to a native shadow
UAV using the payload as an array-layer index. Reusing those handlers as a
boolean visibility query would write the wrong texture/layer. The any-hit
handler also depends on native material, vertex and index bindings.

The new library has private visibility results and custom hit/miss handlers.
Its future state object must associate a verified alpha-test any-hit handler
with the matching native local root data, geometry/SBT indices and instance
masks. These are not yet verified for the main-view spotlight pass. Reflection
scene availability does not establish complete shadow-caster coverage.

R64 captures serialized root signatures, DXR libraries and the known main
direct-light compute shader inside one session log. It records state-object
root/export associations, hit groups, sampled TLAS descriptor creations and
ray dispatch/table metadata. Those records prove observation/command recording,
not GPU execution, complete scene coverage, or matching resource lifetimes.
Root/state/scene/dispatch metadata is bounded; see the counters in the log.
Opaque pipeline-state-stream creation and AddToStateObject are not observed.
If a shader/root is created before hooks install, its bytes can be missing.

## Run once

1. Close Control, install the package, then launch in Borderless.
2. Load the problem room with FG, RR and glass enhancement off. Start with RT off
   to collect the known non-RT direct-light shader.
3. Enable native RT Contact Shadows and RT reflections in the game's menu.
   Return to gameplay for 10 seconds. Move the camera and Jesse briefly.
4. Press F10, then SAVE LOG. Send the single Lighting-R64 log shown by the dialog.

No PIX capture is required. No restart is needed between these settings if the
game applies them live. A missing RT pipeline will be reported rather than
treated as a successful replacement. The R62/R63 atlas-expansion hook is no
longer installed, so this build does not reserve the extra shadow-atlas memory.

## Prototype boundary

Tasks contain a world-space receiver, normal, light center, emitter disk axes,
explicit bias values, sample count and instance mask. Output is one visibility
and validity flag per task. The adapter must retain the stable light identity
and replace that light's native visibility before diffuse/specular accumulation.
Multiplying a combined scene image by a shadow mask would double-shadow other
lights and is not an acceptable integration.

The sampler supports 1–64 rays; one ray targets the center, more rays sample a
fixed equal-area disk. Rays end before the sampled emitter. Alpha testing is
not disabled. The adapter supplies verified SBT hit offset, geometry stride and
miss index. Its dedicated SBT must preserve the scene's existing per-instance
contribution indices; it must not use the native reflection SBT as-is.
Invalid tasks/results retain native visibility in the CPU composition policy.

Remaining before an in-game RT replacement: verified native geometry/material
binding adapter, per-light task production and composition, GPU resource/queue
lifetime handling, temporal/spatial denoising, moving/alpha geometry coverage,
live mode switching and measured GPU cost. The prototype alone is not denoised.

## Validation and recovery

The Windows build compiles the DXR library. Contract tests check layout, bounds
and native fallback. Observer tests check unchanged arguments/return values,
deduplication and file persistence. The extractor rejects partial/corrupt logs
and verifies full SHA256s. These do not validate in-game ray visibility.

Extract locally with `python tools/extract-rt-shadow-capture.py <log> <output>`.
Capture limits: 2 MiB per blob, 8 MiB unique raw bytes and 128 unique blobs per
session. Hex encoding increases log size. Blobs are flushed once per creation;
there are no per-frame resource readbacks. Logs contain game shader bytes and
process-local object/GPU addresses, and should be shared privately for analysis.
