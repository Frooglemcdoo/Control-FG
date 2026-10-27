# R178 — reuse exact-byte shader validation during RR guide preparation

R177 is rejected: its matched-settings capture recorded 39.85 FPS versus R176's 41.9. Shading rose from 5.788 to 6.090 ms and the SPEC-sensitive ray pass also rose from 5.795 to 6.666 ms; those runs do not isolate the entire regression to the probe-load move. Production generation now restores the exact R172/R176 serial shader (SHA256 c4bebcb782c972985e77ea22c04dd8301832b36f863379f3f041a1801ed02889), retaining R176's accepted rejection-fallback reuse. R177's generator/audit remain historical only.

## Target outside SPEC

R176's capture reports approximately 0.6 ms of CPU preparation per frame for RR textured guides. Each preparation creates fresh validation Sessions. Although shader pairs and captured pixels are deduplicated, distinct pairs can parse identical vertex and pixel bytecode repeatedly. R178 caches successful parser results within each Session using exact byte-vector equality plus shader stage. Fresh native bytecode copies, descriptor checks, original/replacement vertex equality, pair limits and copy budgets all remain. No shader is accepted from a pointer identity or hash alone through this new cache.

Each cache is limited to 64 entries and 512 KiB of bytecode; there are separate character/hair and world Sessions. Overflow or allocation failure falls back to normal parsing. Failed parses are not cached. Both caches are destroyed at the end of preparation, with no cross-frame reuse, locks or resource ownership changes. GPU shaders, batches, guide textures, skin/hair admission and rendering remain the R176 baseline.

The extra byte comparisons and copies have a cost. The change may improve CPU preparation if enough validation is redundant; it may have little FPS effect when GPU-bound or regress if reuse is low. No measured speedup is claimed. Logs include RR_VALIDATION_CACHE hits, parses and retained_bytes; existing RR_PERF_FRAME replay_prepare_cpu_ms measures the relevant CPU duration. Replay worker timings are sums across callbacks and must not be added to GPU frame time.

Other observed costs outside SPEC include roughly 3.3 ms for RR evaluation, 1.18 ms for the source-identified native GI-related compute shader and 0.54 ms for another compute pass. These are separate profiling candidates, not changes included in this build.

## Validation

Differential tests compare cache responses, complete contracts and error reasons against the unchanged parser using four captured pixel shaders and a native vertex shader. Tests cover repeated bytes, changed contents at the same address, stage separation, truncation/corruption, disabled/full cache fallbacks and session isolation. The baseline audit reverses only the explicitly listed cache integration edits and still checks the original protected files. Windows build, existing WARP checks and shader validation remain mandatory. No performance or full-scene image equivalence is inferred from these tests.

## Test

Compare R178 against R176, not R177. Restart; explicitly select GI 6, SPEC 16, NATIVE BLEND, RR on, FG off. Use the same view, resolution and scene. Let the image settle, run PERF CAPTURE, and send the ZIP and FPS. The capture contains cache counters and preparation timings so we can distinguish useful CPU savings from a shader rollback. Keep R176 if R178 regresses. Experimental branch only, no production/release update.
