# R175 — restore accepted serial shader and separate ray-dispatch evidence

R174 is rejected following the user's report of lower FPS. Its Save Log confirmed one successful direct replacement creation and zero library loads/failures, but did not include GPU timing data. This does not identify the GPU reason for the regression.

R175 restores the exact R172 serial shader (SHA256 c4bebcb782c972985e77ea22c04dd8301832b36f863379f3f041a1801ed02889). The two R174 arithmetic guards are absent from production. Their tests remain historical validation only. R173's native-fallback cache support and R174's Save Log creation counters remain. SPEC 16, GI, clamp, accumulation, lighting, skin/hair guides and dispatch dimensions are unchanged.

## Why this capture is needed

The R172 capture has 21 non-shadow ray timing records across seven sampled presents. Three dispatches per sampled present share the same 2560x1440x1 dimensions and are pooled as other_rays. A pooled mean cannot identify which dispatch should be optimized. The reflection resolve is separately source-identified and averaged 6.125536 ms over seven samples.

R175 appends ray-generation record address/size, hit-table address/stride, miss-table address/stride and a CPU recording index to each timing CSV row. These values come directly from the intercepted DispatchRays descriptor. No GPU buffer reads, new timestamps, waits, extra rays, or shader instructions are added. Metadata is copied only for the already sampled dispatches. Compute rows have zero ray-table values. Existing columns remain in place.

Addresses identify table locations within a capture, not shader content or stable identities across restarts. Tables may be reused or updated. Recording indices are not GPU execution ordering. Interpretation must check repeatability across sampled presents and correlate with SPEC scaling, without assigning unsupported shader names.

The timestamp WARP test checks descriptor-copy ownership, all new fields, zero defaults for non-ray samples, and recording indices alongside existing fence/reset/replay protections. The existing serial shader audit and Windows package validation still apply.

## Test

Restart after installing. Use NATIVE BLEND, GI 6, RR on and FG off. Keep the same scene, view and resolution. Run PERF CAPTURE at SPEC 16, then SPEC 8, then SPEC Native; allow the image to settle after each change. Send all three ZIPs and note any visual difference. Each capture changes no settings and automatically closes the overlay before its settling period.

This is a baseline restoration and diagnostic build. It does not claim a new optimization or measured FPS gain. The comparison should identify which ray-table groups and compute stages scale with SPEC count before another shader change.
