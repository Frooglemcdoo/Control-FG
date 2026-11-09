# R213 — Native Blend GI stage isolation

R212 was active in the supplied 10:50 log: 23 sampled applied dispatches, 4,626 successful RR evaluations and zero failures. The user reports unchanged ghosting, with boiling and speckles still absent. This build isolates stages rather than tuning another history threshold.

F10 has a new GI button. It cycles these modes, starting at BOTH on every launch:

| Button | Temporal history | Spatial passes | Current-frame energy clamp |
| --- | --- | --- | --- |
| GI: BOTH | R212 adaptive 70% maximum | Native (normally 6) | Retained |
| GI: TEMP ONLY | R212 adaptive 70% maximum | 0 | Retained |
| GI: SPATIAL ONLY | 0% | Native (normally 6) | Retained |
| GI: CLAMP ONLY | 0% | 0 | Retained |

Use Native Blend, RR ON, FG OFF, SPEC Native, hit distance OFF. In each mode, close F10, wait 10 seconds, repeat the same walk across the patterned carpet, then inspect dark shadows and the speckle area. Record whether ghosting, boiling, and speckles occur in each mode. Cycle back to BOTH to check repeatability. Save the F10 log while RR remains on. One log contains requests and actual per-stage application. No F9 sequence is needed.

This is a diagnostic comparison, not a claimed ghosting fix. Temporal-off variants may reproduce the previously observed shadow boiling. CLAMP ONLY retains the current-frame energy clamp; it is not the full GI bypass used in R206. Differences between these modes distinguish temporal contribution, spatial smoothing, and the filtered input's interaction with RR. A visual difference alone does not prove the underlying motion-vector or RR history cause.

Implementation: preserve R212's adaptive bytecode exactly for BOTH and TEMP ONLY. A second immutable PSO differs at only the native 0.7 history coefficient, replaced with zero. Original engine shader records, native provider writes, texture lifetimes and history copies remain. The actual native diffuse-filter function receives zero spatial passes only for TEMP ONLY / CLAMP ONLY. Its audited control flow dispatches temporal at renderer+0x156d3, reads the requested pass count at +0x156e1, and jumps over the spatial loop to +0x15a42 when zero. Native output/history copies follow that branch. Source evidence: research/native-integration-audit/evidence/diffuse-filter.asm.

The UI request is atomic and captured once per eligible GI filter invocation. Temporal choice is thread-local for that scope, restored in a finally block. RR OFF and other lighting modes retain their existing paths. The mode is diagnostic and not persisted. Current-frame clamp, R212 color limits, native root/bindings and exact PSO restoration are retained. No history clears or CameraCut overrides.

The log identifies stage, temporal request, original/effective spatial counts, applied dispatch count, pipeline readiness and RR selection. applied=0 means the requested shader was not used; do not interpret that interval as a successful isolation. The existing outer GI validation still reports the producer's original pass request; use the R213 stage trace for effective spatial passes.

Validation covers all four policies and exception/fallback paths, exact zero-history shader delta, real WARP PSO creation, actual production dispatch selection and native restoration, and GPU pixel readback confirming stale bright/dark history contributes zero in the new variant. Full build checks must pass before delivery. GPU fixtures do not establish in-game quality or the native engine's integration behavior.

Accepted projection rays, MAP GUARD OFF, angled reflections and other R205 defaults remain. Production unchanged.
