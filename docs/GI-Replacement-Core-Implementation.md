# GI replacement implementation checkpoint

The user authorized replacing GI sampling and reconstruction to address both
hair speckles and jaw/neck noise. Preserve native ray budget, material/direct
lighting detail and accepted projector/desk/plant appearance. No repeat generic
capture is requested. This checkpoint is real implementation, but NOT a complete
replacement or a gameplay build.

## Implemented

`src/gi_reuse/frozen_world_guide.h` implements a C++/HLSL sampler matching the
screened 8x8 equal-area world guide with a 1% uniform guide floor. The separate
cosine mixture retains at least 50% native support. Frozen float CDF intervals
define the actual density, with a six-step binary search. Sampling does not
scan all 64 bins for every ray; full validation is separate from the immutable
table's sampling path.

The result includes direction, actual proposal PDF, radiance weight, frame,
epoch and whether to trace. Apply the weight to ALL resulting GI radiance,
including misses, never just spotlights. Preserve native finite-range sampling
and hit-distance semantics separately. Below-hemisphere proposals contribute
zero and stay in the sample denominator; do not retry them.

Guide validity is checked before drawing a ray. Wrong frame/epoch, invalid or
insufficient training and mismatched receiver keys reproduce the original
native cosine sample using the same random coordinates. No post-hit rejection
or selective endpoint replacement is implemented.

Receiver keys include complete signed world-cell coordinates and the normal
octant, with finite/range checks. A hash slot is not sufficient identity. Keys
are CURRENT-frame training groups only, not temporal surface identity. The
0.25-native-unit grid matches the offline candidate; it is not validated as
the optimal live resolution.

## Validation

- Linux C++17, optimization and all warnings as errors: three million draws.
- Constant and directional analytic integrals, rotated/flipped receiver normals,
  proposal support/weight bounds, zero-contribution samples and endpoint cases.
- Invalid training, stale-frame and wrong-receiver fallback preserve the exact
  native random draw instead of drawing an alternative after seeing its result.
- Windows DXC cs_6_0 compilation with warnings as errors.
- Windows D3D12 WARP dispatch: 4,096 CPU/HLSL numerical and metadata comparisons,
  including full-key mismatch, stale frame and invalid/insufficient training.

Latest code commit: 90b1f7751036113d03732af15e4238bf1f100f53.
Successful Windows workflow: 43054871871, Validate GI replacement core.
Earlier workflow 43054372212 exposed HLSL 2018 loop-variable scope warnings;
these were fixed, not bypassed. Both subsequent validation runs passed.

WARP is a software implementation and supplies no RTX 5090 timing or gameplay
quality evidence. These fixtures do not test live descriptor ownership, new
visibility rays, moving-surface reconstruction or the visual defects.

## Remaining integration, in order

1. Establish per-command-list ownership of the two disjoint ray phases, the
   exact native bindings and complete output publication. Native ray dispatch
   is currently in `gi_live_toggle.h`; the later compute evaluator substitution
   is in `gi_evaluator_runtime.inl`. Do not assume both bindings coexist.
2. Build/freeze the training distribution between phases. Carry each sampled
   weight in a dedicated validated resource; do not derive it from quantized
   hit locations, overwrite native texture-coordinate channels or consult a
   mutable later distribution. Trace/shade each receiver once in total.
3. Measure extra guide and scheduling cost before promoting a gameplay build.
   A full native dispatch followed by replacement rays violates the ray budget.
4. Add independently validated moving-surface GI history. Old owned/dual
   history shaders have depth/motion inputs but lack previous normal/material
   identity validation. Their current spatial averaging and history policies
   must not be re-enabled as if they were the requested new implementation.
   Current-frame world-cell keys are specifically insufficient history proof.
5. Validate integrated sampling, weighted output, history invalidation and native
   fallback, then package a test with one clear baseline/replacement comparison.

No runtime include references the new core yet. R424 gameplay remains unchanged.
The first replacement build is not ready; no complete noise fix is claimed.
