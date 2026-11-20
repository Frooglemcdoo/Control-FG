# Control GI/RR investigation — local PIX

Read README.md and the selected run's manifest/logs first. Do not upload, modify,
delete, or replace the original capture. Analyze locally in a new output folder.

User evidence: transient red/orange speckles on Jesse's face/neck/hair during
movement through GI lighting. RR OFF removes red speckles. Higher ray counts help
but do not eliminate them. REBLUR/RELAX caused unacceptable ghosting or detail loss.
Approved Owned GI, reflection clamp (60%), guides, lighting and streaming must be
preserved. No more speculative clamp changes before tracing the artifact.

Source: Frooglemcdoo/Control-FG, work/rr-restored-r489.
R490 rendering/capture commit: e31ce8b3660ab3637dd0ab7de46cc956c03bd047.
R490 repairs custom capture admission; it is not a proven lighting-speckle fix.
PIX runs with custom diagnostic launch flags removed and the normal mod path.

First establish:
1. Capture opens/replays on this GPU/driver; record PIX/driver version and errors.
2. The speckle exists in a captured frame AND remains visible during replay.
3. RR actually evaluated: requested UI state is not sufficient. Correlate available
   mod RR_RESTORED_R489 status logs and the captured pass/resource sequence.
4. Inspect every presented frame in the multi-frame capture. One exported screenshot
   is not evidence for all frames. If nothing is visible, report the absence without
   concluding the game is fixed. Let the user identify missed candidate pixels.

At confirmed pixel/frame coordinates, identify the earliest anomalous stage:
raw GI/ray contribution, GI resolve/clamp, pre-RR composite, RR inputs/guides,
RR output, later postprocessing. Trace resource bindings and writes. Match buffer
resolution, viewport, jitter, exposure/color encoding, and actual motion-vector
conventions. Do not compare the same screen coordinate across moving frames as if
it were the same surface. Ordinary red hair/highlights are not automatically noise.

Use installed pixtool --help and supported PIX API documentation. Event-list and
RTV0 exports are only a starting point; do not invent UAV/resource export options
or assume RTV0 is the final frame. PIX UI DXR inspection may have functionality
that its scripting API does not expose. Report that gap explicitly if it blocks work.

Produce annotated evidence and a report with event IDs, resource IDs, frame/pixel
coordinates, measured values, reprojection checks and unresolved hypotheses.
If PIX cannot faithfully replay RR, establish that from its logs/outputs and choose
a targeted fallback. Do not remove RR to make replay pass and then diagnose an
RR-on artifact from that changed render path. Do not silently reset game settings.
