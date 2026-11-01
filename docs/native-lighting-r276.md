# R276 — reflection clamp comparison

This diagnostic adds an F10 button: REF CLAMP: 60% / REF CLAMP: BYPASS.
It defaults to 60% on each game launch and is not persisted. Bypass selects the
existing raw reflection-copy path; it is not a 100% clamp setting. The fixed
60% shader and all GI/lighting shader arithmetic are unchanged.

## Test

1. Install beside Control_DX12.exe with the game closed, preserving folders.
2. Run ControlTOH/Diagnostics/Start-Reflection-Clamp-Test.cmd.
3. Use Native Blend, GI RAW BYPASS, RR ON, RT indirect ON, FG OFF.
   Keep the same sampling settings and leave the desk lamp intact.
4. Observe the original plant and hair with REF CLAMP: 60%.
5. Switch to BYPASS, close F10, and allow several seconds for RR to settle.
   Keep the same view/zoom. Check stationary speckles and slow camera movement.
6. Return to 60% and confirm whether the difference repeats.
7. Exit normally and send the packaged logs and your comparison/video.

No F7 or CAPTURE RR is needed. This launcher disables GI tracing, so no trace
buffer/readback is allocated. The change does not disable RR or GI, change
sample counts, alter lights, or enable history in the reflection clamp. It does
not reset RR history when toggled; wait for the transient to settle before judging.
The test may reveal more raw reflection noise; this is not a permanent removal.

The UI label is the requested state. Logs verify the render path:
REF_CLAMP_R276 requested=0/60, RR_CLAMP_STRENGTH_CS3_RAW for raw bypass,
and RR_CLAMP_STRENGTH_CS3 requested=60 effective=60 for the normal clamp.
Use Native Blend with RR active; modes preserving native specular filtering
are outside this comparison. Existing automatic reflection suites retain their
own settings, and the button is blocked during captures/suites.
