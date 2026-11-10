# R406: split the remaining shader-application cost

R405 installed successfully and handled 163840 tables, all batched, resolving
491520 current descriptors. No native-table or observer fallback was logged.
This removes 327680 copy calls but does not establish a frame-rate improvement.
Warm dynamic CPU averages were 0.836731 ms/job versus R404 0.846891 ms/job;
provider updates were 0.791681 versus 0.801521 ms/job. Workloads differ.
R405 ordinary application samples averaged 24.795790 us after the first
16384 calls; provider samples averaged 28.658145 us/update after 512 jobs.
These are different populations, not a valid percentage decomposition.

Prior log inventory and R403/R404 detail contain no split inside application.
Native D3D 10570 already checks dirty versions and allocation validity before
uploading. Native 132D0 includes texture lifetime callbacks before UAV copies.
Skipping either globally would remove required work; no evidence justifies it.

R406 adds optional phase boundaries to the existing owned application. A null
probe does not read a clock. One application in 256 gets a stack-local detail
record; phase changes attribute elapsed ticks to the previous phase. No timing
calls are made inside descriptor or constant-buffer loops. Reported phases:
other, retrieve, shaders, eligibility, constants, CBV, resolve, copy, UAV, native.
All phases in one detail report share the same sampled-call population.
Timers preserve GetLastError. Durations include timer/observer overhead and
possible preemption. They do not measure GPU execution. Ordinary whole-call
samples retain the one-in-32 schedule except detailed calls are excluded (7/256).
Do not add their cumulative values to detail values or use the same denominator.

No new native patch sites, no retained pointers or resource reuse, and no
change to native calls, batching policy, queue/barrier/AS semantics. Existing
byte/hash guards and lighting-observer fallback remain intact.

The 4096-case native machine-code oracle alternates null/enabled probes and
checks phase attribution against the actual native callback event sequence,
alongside identical outputs and calls. The runtime header is syntax-checked
with local Windows/D3D declarations before the full Windows build.
