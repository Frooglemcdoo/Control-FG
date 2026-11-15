# RR work resumed from R434 — 2026-11-15

## Baseline selected by user
The user explicitly selected R434 after rejecting both R437 and R438 in gameplay.
The active branch is work/rr-r434-baseline, rooted at
cfcc1f0745701457787e15945131f63ec8ddf5a3.
Do not merge R435–R438 rendering changes into this baseline.

The original ControlTOH-GI-Guided-Live-R434.zip was recovered and its ZIP
integrity verified. Its embedded build receipt matches:
- Windows workflow: 43479439058, success (confirmed through GitHub).
- Source: cfcc1f0745701457787e15945131f63ec8ddf5a3.
- dxgi.dll SHA256: 12bb079c33adc453156f28ecfc3eeecb29a7692c19308f4d2b41c1d5cc0571f3.
- Recovered ZIP SHA256: 2d7b48ce5526add3faa76fbc3ff1915f4feac0ef008c18344c6b6642474c0ee9.
The recovered package includes updated documentation; its executable hash
matches its original build receipt. Do not rebuild just to restore this binary.

## What R434 actually does
src/rr_gi_stage_policy.h defaults to RawBypass. The BOTH mode remains available
as a diagnostic choice; it is not the default baseline.
The guided ray and evaluator implementations are fixed to two world-space bounds
at the recorded lamp scene. They are not an automatic scene-wide guide.
tools/gi_guided_live.py changes directions only when the shader ray count is one.
src/shaders/gi_guided_lighting.hlsl applies the matching weight only for rays==1.
Other counts retain owned directions and the unweighted owned evaluator.
The ON label reports the request/readiness, not the effective per-dispatch count.

The latest compact bundle saves GISamples=4. This is a saved configuration,
not proof of the exact count used in the earlier video. GISampleComparison
preserves the engine count for Native (request 0), and accepts a request of 4
during full RR. Do not assume Native necessarily means one or two internal rays.
Do not change the requested native-ray baseline to force the guide to work.

## Rejected later experiments
The user reports the same increased speckles and boiling from R437 and R438.
Video creation metadata places the supplied clip in the earlier R437 session.
The subsequent R438 session records correction calls 1–2400 across frames
2233–4632, with matching frame/call increments and no logged fallback.
This confirms consistent command recording, not independently measured GPU output.

R438's threshold is discontinuous: on a flat 5x5 surface of 0.01 with a central
sample moving from 0.0999 to 0.1001, its center output moves from 0.0999 to
0.013604. The neighboring outputs jump from 0.01 to 0.013604. The flaw is
reproduced in the CPU reference; it is not proven to be the sole cause of the
gameplay regression, and cannot by itself explain the independent R437 result.

## Investigation boundary
Continue at the source/actual RR input contract from R434:
1. Establish effective sampling count and guide route from existing captures
   before interpreting ON/OFF comparisons.
2. Verify correspondence of ray proposal, PDF weight and evaluator receiver,
   including per-ray/per-pass random coordinates and hit-distance semantics.
3. Trace motion/depth/jitter and material-guide consistency at the actual RR
   dispatch for static receivers and animated Jesse.
4. Locate the first unstable stage before introducing another filter.
No restored baseline, fixed-scene guide or synthetic test is a verified RR fix.
Keep the final 1–2 FPS optimization goal, not a hard cap during investigation.
