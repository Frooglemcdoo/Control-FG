# Coverage candidate: Windows validation completed

Validated commit: 4e6b35fa4be9804b1c7a514081d377159d9a6932 on isolated branch work/character-coverage-validation-r419.
Workflow: https://github.com/Frooglemcdoo/Control-FG/actions/runs/42933550050
Job: 130237922726. All steps succeeded; decoded job logs inspected.

Results:
- 6,144 native remap cases and 36 emitted guard cases passed.
- Eight Direct3D12 WARP dispatch/readback cases passed using the candidate's exact emitted guard around the existing emitted filter. Includes valid zero selector, invalid sentinel, unsupported nonzero selector and MSAA fallback. Invalid selections returned native expected color; valid selections retained reconstruction.
- All twelve character families, their stripped containers and all quality/diagnostic variants passed WARP graphics-pipeline validation. Candidate shader generation includes the current HQ rig changes.
- Native instructions are recovered exactly by reversing the edits; resource bindings remain unchanged.

WARP is software Direct3D12 validation, not a measurement on the user's RTX 5090. No FPS or in-game visual improvement is established. This branch does not modify the production generator or deliver a new game DLL; R419 remains the user build.

The guard has now progressed beyond a CPU-only prototype. Whether unsupported selection occurs on visible noisy pixels remains unresolved. Inventory searches across locally available character-review/evidence-review ZIPs found no named coverage/reconstruction/constant-buffer exports; this does not assert that every possible unnamed blob has been decoded. R378 stride-four buffers cannot resolve 2x2 source ownership.

Do not ship this as the jaw/hair noise fix solely because these tests pass. A subsequent runtime integration should be described as a conservative eligibility correction, with visual impact unproven. Stronger destination reconstruction still requires trustworthy coverage/depth correspondence, especially for hair with native depth offsets.
