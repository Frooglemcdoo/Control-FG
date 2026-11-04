# R324 — complete-batch GI color and exact-witness CPU lookup

R323 capture PID 72736: all 15 indexed files verified by length and SHA256. Every reported queue status remained healthy through the final checkpoint, with 983 admitted passes. No recording failure or admission rejection was logged. Initial geometry censuses passed 7572, 7572 and 11379 entries; the adjacent cache reused 2883, 2883 and 3559 conversions. This is CPU admission evidence, not independent GPU-output validation. The user reports unchanged FPS and more speckles.

Across reported CPU windows: preparation 7.866 ms, admission 13.682 ms (selection 2.528, mapping 7.739, table commit 2.925). Scopes are inclusive, not GPU measurements or additive frame time. Geometry preservation averaged 0.327 ms per build over 33309 calls. Ray count changed from one in the initial recorded passes to six by writer 600; comparisons are not a controlled fixed-workload benchmark.

The initial reservoir path shades all configured rays, then previously displayed one randomly selected, weighted surface sample plus miss fallback. Without spatial/temporal reuse, that selection adds avoidable conditional RGB variance compared with summing the already-shaded batch. R324 retains the reservoir for future reuse but resolves displayed RGB from all validated samples using each sample's existing PDF, BRDF, configured-count and depth normalization. Misses and black samples retain their place in the denominator. Hit distance remains the all-ray mean. No extra rays, clamp, history filter or resolution reduction is introduced. At one ray this change is equivalent; it cannot remove one-ray sampling variance or prove all observed speckles came from reservoir selection.

NVIDIA's integration guide describes initial-only reservoir shading as a math-validation mode and inserts spatial/temporal resampling afterwards. This correction is a complete-batch reference estimator, not completed ReSTIR GI:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md

CPU correction: ResolveExact previously performed a position-allocation search for every triangle in an index-address bucket even when its identity already failed the frozen witness. It now performs that search only for otherwise matching candidates. The first-three-pass diagnostic census retains full evaluation. Exact allocation identity, overlapping-owner rejection and geometry admission remain mandatory.

Validation: analytical unequal-PDF RGB hit/miss/black batch tests prove output is independent of the selected reservoir color; GPU output fixture deliberately supplies a different selected color, tests complete color and hit distance, malformed inputs, disarm and untouched pixels. Existing scene tests cover identity, bounds, stale ownership and both index widths. Windows CI must pass before distribution.

Install preserving folders and run ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd. Keep RR, RT indirect lighting and GI RAW BYPASS. Retain the same ray-count setting while toggling NATIVE/OWNED in F10. Check plant/hair speckles while still and moving, FPS and sustained OWNED status. Exit normally for the automatic ZIP. Existing captures were reused; no RR capture or standalone hardware retest is required.

Limits: native GI still executes for fallback, CPU admission remains costly, and no spatial/temporal sample reuse is enabled. This build is not a claim that performance or all speckles are fixed.
