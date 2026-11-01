# R277: complete GI ray contribution trace

This diagnostic closes a specific gap in R275: that trace recorded partial
spotlight terms, not the complete ray contribution or its final normalization.
R277 records those quantities together. It is not a visual correction.

## Run once

1. Copy the package over the game installation, preserving the folders.
2. Start `ControlTOH/Diagnostics/Start-GI-Ray-Trace.cmd` with the game closed.
3. Load the original plant/hair view, lamp intact. Keep RR and RT indirect ON,
   Native Blend selected, FG OFF. Select GI RAW BYPASS and REF CLAMP: BYPASS.
4. Press F7 or click ARM GI TRACE. The visible five-second ARMED countdown must
   appear. Close F10, keep your aim/zoom steady, and wait for SAVED.
5. Exit normally. Send the ZIP files produced by the launcher.

Mouse zoom may remain held. Do not use CAPTURE RR or PACKAGE RR for this probe.
The existing R275 trace and R276 A/B remain valid; this captures different data.

## Scope and safety

Only the exact hash-pinned GI6 shader and audited native root signature are
accepted. The probe adds private-register stores into the existing diagnostic
UAV. Removing the additions recovers the original shader container byte-for-byte.
Every native lighting instruction remains unchanged, including attenuation,
shadows, projection, sample count, material weighting and output writes.

One pixel per 32x32 cell is sampled. Every executed ray at those pixels is
recorded, followed by the final result. Records include primary/secondary
material IDs, world-space consumer geometry, primary throughput, secondary radiance,
weighted per-ray contribution, running sum, depth-fade constants, configured
ray count, actual depth scale and final float32 GI before UAV conversion.
Secondary geometry is saved immediately after the hit loads, before later
shading branches reuse those registers in view space. Miss normals are zeroed
in the diagnostic output, never reused as surface data.

The 65,536-record cap and overflow counter remain. At 2560x1440 with six rays,
at most 25,200 records are needed for one full dispatch. Overflow, duplicate rays,
missing summaries, mismatched counts and nonfinite data are rejected by the
analysis. Output ordering does not matter. GPU completion is required before
SAVED, and the collector includes only the selected process's trace.

R277 shares R275's tested root restoration, visible arming, mouse-transparent
status banner and asynchronous fence handling. Normal launch does not enable
the probe; the new launcher selects it through an environment variable.

## Limits

This is one sampled dispatch, not a temporal variance/reference accumulation.
It observes the consumer's coordinates, not an exact ray-generator PDF.
Secondary shading can depend on the original incoming direction; a future
reuse implementation must account for that rather than treating these values
as automatically direction-independent. Separate diagnostic multiplication
and native fused accumulation may differ by floating-point rounding.

No game-wide speckle reduction, performance improvement or resampling integration
is claimed. GPU fixture tests validate the inserted blocks and PSO creation,
not the complete game scene or the diagnosis.
