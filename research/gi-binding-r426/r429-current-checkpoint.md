# R429 fallback/status correction

User rejected R428: no motion-noise improvement and substantial FPS loss.
Existing compact logs 20261112-101846 were collected after fixing the collector.
All three probe logs are complete (no trimming); see r428-rejected-session-summary.json.
Latest process requested preparation, built compute/ray pipelines, and accepted
live bindings. It later recorded ordinary owned and native dispatches.
This proves fallback at those events, not that guided work never ran.
R428 status was suppressed by the on-demand logger. Stale R425 build labels
and hardcoded deferred_dispatch_enabled=0 are not runtime evidence.
No per-pass timing or exact terminal rejection survived. Do not invent a cause.

## Implemented correction

- Preserve bounded R428 producer/consumer/rejection records and actual R429
  installation/stop state with diagnostics off; always emit terminal reason.
- Eight consecutive explicit rejections stop further guided attempts; a
  successful producer resets the streak. Fatal partial failures stop immediately.
- A recorded producer without a matching consumer at scope exit now stops
  retries rather than repeatedly doing work with no completed guided output.
- Stop new frozen source registrations when stopped or installation incomplete.
  Do NOT clear the registry or turn off retirement callbacks while owners exist.
- After queue submission and recording seals, collect only proven-retired
  owners and trim idle allocations while halted. Both recording seals and
  actual GPU fence retirement remain mandatory. Failed/pinned owners stay pinned.
- No ray count, sampling algorithm, RR or RELAX tuning changes.
- Normal launch remains outside the guided path. R429 launcher is opt-in.

## Limits

Not a confirmed noise fix or measured FPS recovery. Eight-attempt bound is a
conservative session circuit breaker, not proof that an underlying rejection
has been repaired. Native/owned menu toggles do not rearm a stopped session.
Some observer and scene-window callbacks remain to preserve safety.
Source-only R428 mode already skipped legacy AS audit; do not attribute the
regression to that audit. Potential real costs include source registration,
preparation/rejection, successful geometry copies/guide passes and scene gating.

## Validation / delivery

Initial R429 Windows core run 43258044469 passed, including native scene gate,
owner retirement, actual reset/destruction, GPU fixtures and launcher tests.
Final source a71b0ada05cc141153ecbc7750c2da9e880ebffa includes terminal-log fix.
Final core run 43258598252/job131323442674 SUCCESS.
Full package run 43258597915/job131323536291 SUCCESS.
Do not deliver earlier cancelled builds. Verify final package CRC, digest,
launcher, runtime markers and DLL hash before saving/delivery.

Verified package ControlTOH-GI-Frozen-R429.zip: 6991503 bytes, 88 members.
ZIP SHA256: 3445ad3fd5c6524b99cbe642e4e63ea3d117096f36d310652e95fa605e1e89cd
DLL SHA256: 690d53c8ba0ba31d6b83679fdaa28a8ec1604b0b44c4b3113bc98ace0b4424d1
Artifact13962224871. Launcher and runtime markers verified.
Next: short in-game run and existing compact logs to establish exact rejection
or guided completion; no repeated R425-427 ordering capture or new video needed.
