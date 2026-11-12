# R432 synchronized stage-capture checkpoint

Base: 7e73d2430e850d185fef28b51294934b31f5c07d on
work/gi-replacement-binding-r426. Implementation branch:
work/gi-stage-diagnostic-r432.

The request was a consolidated diagnostic build after R431 was rejected.
Repository recovery established that R423/R424 already captured bright GI
upstream of RR and decomposed selected source samples. Reuse those findings;
do not misrepresent this work as first establishing the source/RR boundary.

Implemented: opt-in single stationary-then-moving sequence, two consecutive
full-resolution frames per phase, same-frame timestamp pairs for the native
GI evaluator boundary, temporal/raw-copy dispatch, optional spatial dispatch,
and RR evaluation. Existing image capture supplies raw GI, GI at RR input,
HDR RR input/output and guides. The launch script disables frozen-guided
preparation and rendering; native ray budget and existing shaders are unchanged.

The admitted baseline has RAW BYPASS and no temporal/spatial GI denoising.
There is deliberately no fabricated intermediate temporal image. Missing
per-pass timing, noncontiguous/missing evaluator batches, changed frame/queue/
thread/context, timestamp overflow or reversed clocks invalidate timing.
Query storage is prepared off the render thread. Timestamp commands update
the native command count and use the image job's existing same-queue retirement.
Failure after any query recording retains GPU resources rather than freeing
in-flight allocations. Export includes null for unavailable durations.

Limits: these scopes do not measure rays, scene/AS preparation, composition,
RR guide generation or total CPU/GPU frame cost. Captured-frame scheduling
includes diagnostic perturbation even though image copies are outside the
timed scopes. Do not claim steady-state FPS, a noise fix or proven gameplay
capture coverage before testing. R431 guided-path performance is not measured
by the disabled baseline. The final renderer replacement remains unresolved.

Local checks passed: timestamp ledger/coverage/overflow and duration tests,
existing manual-pair policy test, motion-capture contract and reconstruction-map
capture contract. Existing path-policy test was run without GCC -Werror because
its pre-existing compact if formatting triggers misleading-indentation; the new
timing-policy test passes -Wall -Wextra -Werror.

Windows build and on-GPU validation are pending. Automatic approval review
blocked the push to Frooglemcdoo/Control-FG for CI, requiring user approval for
that upload. Do not claim a DLL/package is available. No alternate publication
or API upload was attempted after the block.
