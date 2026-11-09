# R192: temporal-provider observation

This is a diagnostic build, not a visual fix. Rendering and the F9 suite remain
R191. The only new runtime behavior reads and logs named native CPU providers.
No shader substitution, ray-count override, provider write, GPU allocation,
GPU readback or GPU wait is added by this probe.

Install the experimental DLL as usual; keep the NVIDIA runtime files. Enable
RR, select NATIVE BLEND and SPEC 4, disable frame generation, and stay in gameplay
for about 30 seconds. The painting is preferred, but the counter check does not
need exact camera matching. Send the **Lighting-R192-*.log** from this run.
F9 and the large image packages are unnecessary for this check.

The probe automatically records at most 1024 distinct eligible engine frames
per process, starting when RR is selected and the observed producer count is 4.
It then stops. Restart the game for another window if needed. Time spent at
other sample counts does not consume the budget. An invalid provider read does
consume an attempt and is explicitly marked; it is never converted into a
valid zero. This cap bounds logging, including failures.

Each observation resolves g_uTemporalFrame and g_uCameraCut by name through
the existing hash-locked resolver, validates four-byte provider metadata,
reads the current rendering thread's provider arena, verifies it with the
native comparison API and repeats both reads. Values are recorded immediately
before the reflection filter, after a producer sample observation in that
frame. The trace labels this **CPU provider evidence**, not verification of the
GPU raygen constant buffer. It also does not certify successful RR evaluation.

`tools/analyze-rr-temporal-probe.py LOG` splits at missing frames/attempts,
failed reads, sample changes and camera cuts. It requires at least 256
consecutive valid observations before reporting a short observed period
(up to 64) or monotonically advancing uint32 values. A period applies only to
the observed window. Dropped diagnostic queue records produce gaps, not a false
continuous result. No quality/performance conclusion follows from this log.

If a short CPU-provider cycle appears, the next requirement is establishing
that it is the counter consumed by raygen before changing rendering. A
progressing provider weakens the short-cycle hypothesis but does not establish
GPU consumption or adequate angular coverage.
