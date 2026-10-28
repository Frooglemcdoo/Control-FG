# R195: accepted Native Blend baseline and in-place ray capture

This consolidates the accepted reflection configuration and corrects capture
behavior. It is not a fix for the Board Room missing-light report, which is
paused until this baseline is verified.

## Normal launch

Launch normally; no special launcher is required to disable hit distance.
Native Blend is the automatic startup selection once its shaders are ready.
SPEC defaults to Native (engine-selected count; previously observed as one).
Higher sample counts remain optional. Compact hit distance is OFF by default;
only an explicit CONTROLFG_RR_COMPACT_DISTANCE=on enables the old diagnostic.
The R193 long primary reflection sequence, R184 secondary view direction,
R172 immutable load relocation and R176 invariant fallback reuse remain.
The old per-frame temporal-provider probe is no longer enabled in this build.

Keep RR enabled and restore the game settings used in the accepted baseline:
Ray Traced Contact Shadows, Indirect Diffuse Lighting and Debris were ON in
that run, along with opaque/transparent reflections. Contact Shadows is a
required prerequisite for the spotlight/ray pipeline. The mod does not silently
rewrite the game's RT settings or enable RR. Native Blend may remain unavailable
until those pipelines exist. SPEC Native is a sample choice, distinct from the
NATIVE lighting button.

## Capture the actual working configuration

Select NATIVE BLEND with RR ON, then F10 -> TRACE SHADOW. Hold still until the
completion dialog. Send lighting.log, manifest.txt and three PNGs from the
Shadow-R195 folder. No MAP PROBE colors or deliberate RR-off phase is involved.
The capture does not call SelectMode at entry or completion. It adds bounded
diagnostic rays/readback and uses the existing fence retirement; normal sample
count and lighting shaders remain selected. Diagnostic capture itself costs
work, so do not use its interval for performance comparisons.

Admission names missing prerequisites: selected mode, RR, Contact Shadows,
hit-distance configuration, ray/composition/pipeline readiness, previous capture
failure or pending export. Settings are recorded in the manifest. Changes to
mode sequence, RR request, SPEC choice, RT effects or hit-distance configuration
cancel the capture rather than producing a mixed-configuration result. A user
change is not overwritten on cancellation. F9 cancels an active light trace;
otherwise its older separate reflection suite remains available but is not
needed for this task.

The ray diagnostic remains a 64x64 grid of full-length reference rays with
native alpha testing, plus matching selected-light records and before/after
lighting-composition data. It is not every gameplay ray, and reference hits
are not the four-ray filtered shadow result. RR remains selected, but the
captured lighting-composition buffers are upstream of RR reconstruction.
Inspect the log for actual capture completion and RR evaluation results.

## Verification and limits

Tests exercise each admission failure, settings-change detection and the new
hit-distance default, alongside the existing capture, shader and baseline
checks. Windows CI compiles the actual runtime. In-game capture with RR enabled
still needs user validation; a successful build is not proof of that result.
R193 rendering labels may remain on unchanged shader paths. The main log and
capture folder identify R195. Production/main/releases are unchanged.
