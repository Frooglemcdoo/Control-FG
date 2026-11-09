# R404 result: provider updates dominate the recording dynamic stage

Read the entire supplied RT-Assembly-R404.zip log (101004 bytes). All four active
hooks installed, the exact-input cache remained intentionally disabled, and no
error-like lines were found. All 14 detail/coarse reports agree on sample count
and aggregate dynamic timing within integer-microsecond rounding.

Exclude reports through job 512 to avoid startup. Reports 512 to 6656 cover 384
timed jobs and 10577 updates in that sampled population (27.544 updates/job).

| CPU interval | Mean ms/timed job |
| --- | ---: |
| Device retrieval | 0.000133 |
| Common resource binding | 0.000893 |
| Transition into UAV | 0.024513 |
| Provider update loop | 0.801521 |
| Transition back | 0.019831 |
| Dynamic aggregate | 0.846891 |

Provider updates are 94.64% of the dynamic interval, averaging 29.10 us per
sampled callback including loop overhead, child work, and possible preemption.
The common binding helper is less than 1 us/job; replacing it alone is not a
plausible material FPS optimization. Both transition directions total 0.04434
ms/job. This resolves the R404 question without another identical capture.

These are CPU-inclusive elapsed intervals, not GPU durations or proven frame
critical-path costs. Other recording phases total approximately 0.355 ms/job.
Preparation independently averages 2.37237 ms/loop and selection 0.29346 ms/query;
do not blindly add separately measured stages into a frame-time claim.
No FPS series/native A-B comparison exists in this collector. R404 is diagnostic,
and its lower aggregate versus earlier gameplay is not evidence of a speedup.

Continue auditing work inside the provider callback (known skinned path A67B0),
including current skinning/object constants, shader table application, and compute
dispatch. The timer does not isolate those child routines, establish that every
provider is skinned, or prove that unchanged resource pointers permit reuse.
Do not skip animations, reduce update frequency, or add persistent binding reuse
without a complete current-resource/version contract. Existing native guards and
same-shader checks are documented in rt-dynamic-diagnostics-r404.md.
