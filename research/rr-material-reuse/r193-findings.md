# R193 accepted stationary result; motion boiling remains

User reports that R193 completely fixed stationary splatter at SPEC4, but
subtle boiling remains during motion in shadowed reflections. Treat the
stationary fix as accepted and retain its long primary sequence. The user
suspects this motion issue arose during earlier testing; the introduction
point is not established by this upload.

Evidence: Lighting-R193-20261028-065932-50844-0.log,
Screenshot 2026-10-28 070217.png, and 20261028-1203-39.9118635.mp4.
Video is 14.0333 seconds, 3286x2028, 30 fps. Sampled full-scene and consecutive
cropped frames show camera/character motion through dark reflective flooring.
Compression, real reflected motion and viewpoint changes prevent assigning
all pixel changes to noise or quantifying a regression from this clip alone.
The video overlay selects NATIVE, while the painting screenshot selects NATIVE
BLEND. Neither the NATIVE selection nor an RR OFF action button proves RR off.
Do not infer exact per-video-frame RR state from unaligned log timestamps.

The replacement library reports SPEC_LONG_SEQUENCE applied=1 native_fallback=0
at present 5406. The probe has 1024 consecutive valid observations on frames
7814..8837, no cuts/gaps, temporal period four, and current values advancing by
one. This remains CPU-provider evidence; combined with the accepted visual
result it strongly supports retaining the causal sequence experiment.
No FPS improvement is established by screenshots with different viewpoints.

The R190 compact distance path remains active: late log reports 11802 bound
RR evaluations out of 11813 evaluations, copied_layers=1. Source
src/shaders/rr_distance.hlsl reads NativeMaterialId and NativePositionTexcoordY
at layer zero, not an aggregate of the SPEC4 signal. A first-ray guide can
change with sampled hit surface. Motion/reprojection sensitivity is a candidate,
not a proven cause; a longer varying sequence can also expose temporal noise.

Next bounded comparison: keep the R193 DLL and all other settings, launch
Start-Compact-Distance-Test.cmd option 2 (hit distance OFF). This sets only
CONTROLFG_RR_COMPACT_DISTANCE=off for the child game. It retains the R193 shader
sequence. Compare the same moving dark reflection, and verify stationary fix
persists. Do not re-run the F9 multi-stage image suite. If boiling remains,
this comparison deprioritizes the compact guide; do not blindly add history
or repeat previously failed temporal blur settings. No new rendering change
has been made for the motion issue.

## User follow-up: hit-distance OFF and SPEC Native

User reports option 2 fixed the motion boiling and SPEC Native looks just as
good as SPEC4. Accept the reported results; no new log was attached to verify
the OFF run's bindings. Preserve long-sequence rendering and favor compact
hit-distance disabled for the next build. Do not globally generalize that RR
hit-distance guidance is harmful; this result concerns our layer-zero guide.
Previous R193 log records Native requested=0, native=1, effective=1; Native
means defer to the engine count, not disable reflections or revert R193. SPEC4
raises the reflection producer ray budget from the observed one to four per
pixel/invocation. Lower per-frame sampling may have less robustness on newly
revealed regions or moving detail; no regression in these cases is established
by this follow-up. No FPS measurement was supplied.
