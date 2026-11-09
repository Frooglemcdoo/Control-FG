# R310 — Owned GI integration

Install the package over the existing mod, preserving folders. With Control
closed, run `ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd`. Keep RT indirect
lighting and RR enabled and select GI RAW BYPASS. Leave other settings unchanged.
Use the F10 GI button to compare NATIVE and OWNED while standing still and moving
around the lit plant/hair scene. Allow RR history to settle after switching.
Exit normally; the launcher packages diagnostics without a capture hotkey.

`GI: OWNED` means the CPU admitted a replacement pass for submission. It is not
a GPU readback or a claim that the image is correct. `GI: OWNED WAIT` means the
requested path has not been admitted, or its most recent admission failed;
native output remains. Persistent WAIT needs the automatically collected logs,
not another capture. NATIVE disables new owned recordings; already submitted
frames finish normally. The existing sampler stays enabled in both comparisons.

This build connects five owned stages to the real GI6 boundary: finite-range
proposal generation, private DXR tracing with native alpha/material handling,
secondary-hit lighting, initial reservoir estimation and native-format output
resolve. Admission uses the current native bindings and ordered TLAS/BLAS scene,
with build-time copies of index/position geometry. Missing or unverified scene
data leaves native output intact. The current path requires one native GI pass;
other pass counts remain native and are reported in diagnostics.

It is an initial owned producer/estimator integration, not the completed GI
modernization. Native GI6 still runs first for fallback, and no spatial or
temporal sample reuse is active. Full-frame processing uses bounded scratch
tiles; the scratch reservoirs are not persistent history. Additional GPU work
and geometry storage are expected, so this is not a performance release.

Geometry copies and scratch each have a 512 MiB active limit; pooled idle
geometry is limited to 64 MiB. Owners are retained through queue completion.
The submission prefix arms each pass once and the GPU clears it after resolve,
so a completed-list replay cannot rerun that owned pass. The live path adds no
CPU fence wait and no image readback. Logs distinguish recording, admission,
rejection reasons, completed queue checkpoints and incremental compactions.
Scene metadata is compacted between admissions using polled completion stamps;
it does not require every queue to become idle.

The existing RTX 5090 standalone producer result is reused. This package needs
an in-game integration check, not a repeat of that standalone test. Report the
toggle status, speckles, missing lighting, color changes, flicker or frame-time
regressions and send the generated diagnostics ZIP. Launching normally leaves
the experimental owned path off. DirectStorage settings are unchanged.
