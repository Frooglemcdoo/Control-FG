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

Windows CI run 43278316274 passed all build/validation steps for source
3bc264542c369d6591a584d1b7b68cfb715cd254. Earlier run 43276548376 failed
an isolated temporal-filter fixture because capture timing hooks were not gated
for noncapture builds; the correction was compiled and tested in the passing run.
The user explicitly approved upload of this branch and Windows CI.

Final delivered package: ControlTOH-GI-Stages-R432.zip, 6,944,989 bytes.
SHA256: 150c9c7e3227f277552e60b94633d187fbbf9bd9e6a70d3cbb01272ce106f743.
Library ID: libfile_04b74e22f388819198a5630b785dd658.
The CI artifact digest and inner ZIP hash matched. The delivered ZIP normalizes
Windows path separators and includes BUILD-SOURCE.json. The DLL is unchanged
from CI; all 18 external shaders match the prior R431 package byte-for-byte.
New timestamp coverage tests and existing rendering/WARP tests passed. Actual
in-game capture coverage and RTX 5090 quality/performance remain untested.
