# R284: wider, eight-frame GI source capture

This is a diagnostic build, not a speckle fix. Native lighting calculations, RR settings, and the DirectStorage baseline are unchanged.

Run `ControlTOH/Diagnostics/Start-GI-Joint-Trace.cmd`. Keep the desk lamp intact, RT indirect lighting and RR enabled, GI RAW BYPASS, REF CLAMP BYPASS, Native Blend, and frame generation off. Aim the crosshair at the affected leaf or hair. Mouse zoom may remain held.

The red rectangle approximates the 128×128 GI-dispatch region on screen; the white rectangle marks its dense 32×32 center. Position the affected material inside it. Press F7 once, close F10, and hold the camera and zoom steady through the countdown and eight-frame sequence until SAVED. Exit normally to collect the package. Do not use CAPTURE RR or PACKAGE RR.

Each frame samples 4,864 pixels: every second pixel in both axes across the region, plus every pixel in the center. All records for a pixel come from the same native dispatch: complete rays, spotlight candidates and products, point-light products, volume lookup/blending, and the surface lighting sum before material albedo. The eight frames are spaced by readback/export completion plus at least 250 ms; they are not consecutive gameplay frames.

The native shader instruction stream is recovered exactly after removing instrumentation. The diagnostic adds stores and private temporaries, without changing light contributions. There is one instrumented dispatch per captured frame, not a second lighting evaluation.

Two fixed 64 MiB buffers plus a 256-byte header upload are reused only after the owning GPU fence has completed and the preceding frame is exported. There is no render-thread fence wait or growing capture queue. Instrumentation/readback can still cause diagnostic frame-time spikes. Overflow fails visibly and retains the partial frame; it is not reported as a complete capture. Eight uniquely named BIN files and frame/present/ROI log records identify the sequence.

A best-effort desktop BMP near the end of the countdown supplies framing context and the visible ROI. It is not synchronized to a captured GPU frame and is not HDR/color evidence. Exclusive-fullscreen/compositor limitations may produce an unusable reference; trace data remains independently usable. The rectangle uses rounded dispatch dimensions, so it is an approximate screen overlay at padded edges.

The collector includes this process's eight frame files and reference image. Earlier versions' files remain supported. Camera/object motion is not automatically ruled out: temporal differences need the stationary setup and framing context. Light/material IDs remain session-local. Missing point/spot records alone do not prove a measured zero; full surface accounting is checked separately.
