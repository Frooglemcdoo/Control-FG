# Follow-up contract audit on R419

2026-11-11. No rendering changes; no new user capture requested.

## What was checked

1. Executed the captured native character remap instructions for all twelve SHA-pinned families, four pixel parities and 64 randomized packed map words: 3,072 passing cases. This agrees with the 2x2 reconstruction formula used by our filter. Coverage sample zero only, matching the filter's single-sample eligibility. No coordinate-bit-layout defect found.
2. Ran the existing secondary-evaluator suite: six passing tests. All 674 native instruction markers remain accounted for. Reviewed the Owned wrapper: throughput comes from initialized receiver r3.xyz; secondary evaluation does not overwrite that vector; miss lighting and final native depth fade/ray-count division remain present. This verifies static code correspondence, not actual runtime resource/constant-buffer agreement.
3. Executed existing generated sampling arithmetic for the current two-ray Owned case, both variants, 65,536 frames each. Finite unit coordinates and per-frame marginal strata passed. Mean U/V were 0.49951202/0.50007796; angular patch frequency was 0.062561 versus expected 0.0625. This is one fixed pixel/batch statistical check, not proof of independence, good motion convergence, or low variance in actual lighting.
4. The current Owned producer normalizes its basis normal and constructs an explicit orthonormal frame. The older native sampling audit's unresolved basis assumption must not be repeated as if this implementation were absent. Equal-weight strata are not automatically a missing-PDF defect.

Reproduction: tools/test-character-native-remap.py, tools/test-character-diffuse-impulses.py, tools/test-gi-secondary.py. Sampling uses evaluate(variant,2,frames=65536,owned=True) in tools/test-gi-live-sampling.py.

## Concrete destination-validation gap

In tools/character_diffuse.py, block() receives the native remapped lighting coordinate as center_source. It loads its center normal and material from that coordinate. The destination position is used for neighbor addressing, but its normal/material are never read independently. Neighbor guides are compared to the remapped center's guides.

Thus a wrong-surface center cannot be identified by these checks: neighbors matching the wrong center may pass, while neighbors matching the destination may be rejected. This differs from a parity decoding error and from simply needing a stronger smoothing radius.

Added an emitted-instruction reproduction: destination (5,5), remapped center (4,4), map zero. Change only destination normal, mask and material ID. Output is bit-identical and the read trace confirms the destination guides were not sampled. This establishes missing destination validation. It does NOT establish that the native map actually selects a wrong surface in the user's artifact.

The native reconstruction also selects this center. We must not assume its selection is invalid simply because coordinates differ: sparse shading legitimately shares suitable samples.

## Next implementation gate

Inspect whether destination GBuffer/depth represent the same skin or hair surface as the character shading invocation. Opaque skin and partially covered hair cannot be assumed equivalent. If destination guides are trustworthy, prototype destination-anchored candidate validation with depth/normal/material checks and an energy-preserving fallback. Validate on mismatched-source fixtures and matched-surface fixtures before any game build.

Do not blindly remap RR guides, delete the native center, or globally clamp bright samples. Those changes could break legitimate lighting and transparencies. If destination correspondence proves valid already, this gap is not the explanation; return to source variance rather than claiming a fix.

Existing R378 exports are stride-four and rear-view, so they cannot settle full-resolution jaw correspondence. Existing user videos establish motion dependence but do not expose buffer ownership. No claim of a complete visual fix or FPS improvement is made.
