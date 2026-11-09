# R403 user result: seemed worse

Full supplied log: probe-20261109-194758-320-60248.log, 83,491 bytes.
All four active ownership stages installed. Cache disabled as intended. No
error-like lines found. User's performance report remains the acceptance signal;
installation success and native parity are not evidence of a performance win.

Exclude the first 512 jobs (including initial 155.960 ms BLAS/setup outlier).
The remaining 320 timed jobs, spanning reported jobs 512–5632, average:

| CPU phase | ms per timed job |
| --- | ---: |
| Thread setup | 0.052528 |
| Dynamic stage | 0.890331 |
| BLAS/finalization | 0.176406 |
| Material-ID patching | 0.094878 |
| TLAS/finish | 0.031228 |
| Sum | 1.245372 |

Dynamic stage accounts for ~71.5% of sampled recording time. It includes device
retrieval, common resource binding at renderer 10F160, UAV transitions, and provider
update callbacks. Do not call all of it skinning, GPU time, or a wait. The helper
named `flush` in the owned adapter performs common parameter/resource binding;
that internal name does not establish an upload flush or GPU stall.

Matched reporting intervals of 5120 preparation loops processed 7690.83 objects
per loop in R403 versus 6022.46 in R401 (+27.7%). CPU preparation averaged 2.27405
ms vs 1.88140 ms; normalized ns/object 295.68 vs 312.40. These workloads differ,
so neither aggregate nor normalized values isolate regression or establish gain.
No frame-time/FPS series or native/owned same-workload recording comparison exists
in this log. Do not invent a numerical FPS loss or blame the user's route.

R403 preserves all expensive native GPU callbacks and does not eliminate the
measured dynamic work. Another equivalent orchestration rewrite is not a useful
performance step. Use the known dynamic-stage code and prior captures to pursue
actual reduction of repeated bindings/updates while retaining current deformation,
resource lifetime, and transition semantics. Do not request an identical capture
just to confirm this conclusion. R403 is not accepted as a performance upgrade.
