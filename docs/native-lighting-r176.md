# R176 — reuse identical rejected-direction fallback

R175 captures measured 40.6 FPS at SPEC 16, 53.75 at 8 and 69.6 at Native in the user's test. The SPEC-sensitive ray dispatch averaged 6.30/3.11/0.44 ms; serial shading averaged 6.09/2.93/0.43 ms respectively. Sparse timestamps identify scaling, not precise full-frame attribution. R175's log also records reflection library c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e (native variant 0).

In that library, each sample first computes its own direction. If rejected, the fallback restarts its retry index at zero. That search depends on pixel inputs and the immutable frame index, but not the outer sample index. Repeated rejected samples therefore perform the same pure computation. R176 computes the fallback at the first rejection and caches its three direction components and a valid flag for this raygen invocation. Subsequent rejected samples reuse those components. Accepted directions are unchanged. Every ray still traces with its original origin, direction, distance, mask and payload layer. The 100-attempt fallback limit, material/normal/position writes, alpha testing and hit/miss handlers are unchanged.

The extra four values live across TraceRay calls. They could increase register or continuation storage costs enough to offset saved retry work. No FPS gain is claimed. This experiment is independent of the rejected R174 shading guards, which remain absent. R172's accepted serial shading shader, skin/hair guides, RR policy, GI and spotlight behavior remain.

## Scope and validation

Only the exact 39008-byte variant-0 library is eligible, verified by SHA256. Other variants pass through. The creation hook preserves exports and subobjects and rebases association pointers into its temporary descriptor copy. If replacement creation fails it retries the native descriptor. Save Log and PERF manifests include matched/created/failure counters; creation is not proof of GPU execution.

The transformer proves that all fallback SSA inputs are invariant with respect to the outer sample index and that the fallback has no memory side effects. All original arithmetic instructions remain in order. All code outside raygen, TraceRay arguments, and retry cap remain unchanged. Tests execute the inserted PHIs over 1424 rejection patterns, including all eight-ray patterns, long sequences and late first rejection; mutations that introduce sample dependence, memory writes or a changed cap are rejected. Windows CI assembles and validates DXIL, tests descriptor copying and native fallback, and runs existing package checks. These are structural/control-flow checks, not a DXR hardware image-equivalence or performance measurement.

## Comparison

Restart after installing. Compare R176 against R175 at SPEC 16, GI 6, NATIVE BLEND, RR on and FG off, same scene/view/resolution. SPEC resets to Native on restart: explicitly select 16. Allow the image to settle before PERF CAPTURE. Report FPS and send the capture ZIP plus any visual differences, especially rough surfaces and hair. If performance regresses, restore R175. Native/8 captures are optional after the primary 16 comparison.

Experimental branch only; no release or production branch update. The repository itself is public, as previously acknowledged; a branch does not make its source private.
