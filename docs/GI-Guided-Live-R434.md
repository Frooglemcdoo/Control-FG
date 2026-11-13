# R434 live guided GI test — prepared, not Windows-validated

This branch ports the fixed guide from the successful saved-frame experiment into the live native GI dispatch. It is a scene-specific experiment, not the automatic guide builder or a general noise fix. The two bounds target the captured desk lamp, recorder and housing.

## Behavior

- F10's GI control selects `GI GUIDE: ON (FIXED)` or `GI GUIDE: OFF`.
- ON guides 10% of the existing one-ray draws. OFF keeps the existing owned sampler/evaluator, so both comparison modes use the same lighting implementation.
- The native ray origin, range, traversal, hit/miss shader table, and configured ray count remain intact. Multiple-ray configurations retain the existing owned directions and do not receive mixture weighting.
- The evaluator weights complete radiance, including miss/volume contributions. It does not weight hit distance or alpha. Below-hemisphere proposals receive zero radiance weight; this live version still traces them within the existing budget.
- The verified native render callback pairs ray/evaluator selection in thread-local invocation state. A UI change cannot change the weighting of rays already selected in that invocation. Unsupported callback identity leaves guiding inactive.
- No automatic guide generation, deferred scene reuse, second GI pass, history allocation, or GPU readback is introduced.

## Validation completed locally

- Both audited native ray libraries pass the existing sampling audit, including configured counts 1, 3, 4, 6, 8, 16 and 64.
- The emitted guided LLVM instructions were interpreted for 100,000 random/edge cases and compared with an independent vectorized formula. Maximum directional component difference: 2.09e-7. Multi-ray fallback is bit-exact to the input owned directions. Includes coincident bound centers and inside-bound proposals.
- The patched native ray library assembles and passes DXIL validation.
- The weighted evaluator compiles with warnings as errors and passes DXIL validation.
- Full Windows proxy build, DXR state-object creation, live visual quality, motion and performance remain untested.

## Build status

The source is committed locally on `work/gi-guided-live-r434`, based on `a7961968ec10fc1bb09c6ad5a56b330797cd494f`. The existing Windows workflow is configured to produce `ControlTOH-GI-Guided-Live-R434.zip` after its validation suite. The push to the public repository was blocked by automatic approval review; no branch or build has been published.

## Planned in-game test after a successful Windows build

Install the produced drop-in package, launch normally, and use the same desk-lamp location with the lamp intact, RT indirect illumination enabled, and GI rays set to NATIVE. Use F10 to compare guide ON/OFF while standing still and moving. Keep other settings equal, including RR. A WAIT label means initialization is incomplete; it is not evidence of active guiding.

The guide does not move with the lamp or learn other rooms. Destroying/moving scene objects can reduce its effectiveness. Correct mixture weighting preserves the estimator in expectation; temporal stability and apparent brightness must still be checked in-game. The earlier replay timings did not establish a performance benefit.
