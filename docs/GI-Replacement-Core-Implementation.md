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

## R425 live integration blocker and next build

The source review found that existing captures identify ray and GI6 work but do
not join their command-list recording IDs to ExecuteCommandLists. The bounded,
opt-in `gi_phase_audit.h` now supplies that CPU metadata without GPU readback,
extra rays or injected GPU work. See `GI-Phase-Check-Contract.md` for limitations.
R425 remains R424 rendering; it does not connect the frozen sampler to gameplay.

Windows validation 43058104325 passed sampler and metadata tests, including
recording markers surviving ClearState, clearing on Reset, concurrent event
publication and capacity overflow. Validation 43058567089 additionally passed
the actual CMD timestamp and collector tests, including stale-session rejection
and paths with spaces. The Python parser rejects incomplete evidence and never
authorizes native suppression. Full DLL build uses commit
954c767859b3a326a7a826378f5e1c3c045d2973; its validated collector updates are in
72e922435e84fcad09eccfaeb54f550ce658c901. No gameplay noise/FPS claim follows.

The R425 phase-check ZIP has now been received and analyzed; do not request it
again. All 24 target frames have a complete ray/evaluator pair, separate ordered
submissions on the same direct queue, and matching root/heap layouts within each
pair. Root argument values differ in slots 0, 2, 3, 4 and 5. Heap identities rotate
between frames. See GI-Phase-Findings-R425.md for evidence and truncation limits.

## Current implementation checkpoint (November 11)

Core Windows run 43066361835 passed the GPU guide builder, separate immutable
phase-binding values, split-recording planner and GPU SBT snapshot tests. The
SBT fixture overwrites the native upload table after the copy, then reads the
snapshot in a second submission to verify the original bytes survive. This does
not establish ownership of geometry, acceleration structures, descriptor contents
or pointed-to constant-buffer bytes in the running game.

Run 43069119697 passed Microsoft assembly/validation of the compact variant-zero
raygen pair and shared hit/miss shaders. Both hit and miss derive output locations
from DispatchRaysIndex, so the compact mapping must apply to all three exports.
The new A/B payload namespace preserves native, owned and visibility payloads.
A mask mistake was caught by the namespace test and corrected before this run.

The runtime now has an opt-in CONTROLTOH_GI_COMPACT=1 stage. It traces alternate
columns in two immediate half-width dispatches with the original bindings and
shader table; it does not defer work across command lists. Total ray count is
unchanged. Only audited variant zero, even widths and the expected table strides
use the compact path; other cases use the existing full-width owned raygen.
Pipeline or record preparation failure falls back before any ray is dispatched.
The two phases have distinct 64-byte-aligned raygen records and share original
hit/miss identifiers. The ordinary native/owned toggle remains available.

This is a scheduling/parity integration stage, not the noise fix. The frozen guide
is not yet used by gameplay and neither RTX timing nor visual parity is verified.
The compact path is disabled unless explicitly requested. A full DLL build is
required before any package can be described as ready.

Remaining work: connect shade-A/guide-freeze/ray-B/shade-B with proven resource
lifetimes, actual sampled-weight storage, final output publication, then validate
moving-surface reconstruction and real performance. No extra capture is needed
for the already-established scheduling relationship.
