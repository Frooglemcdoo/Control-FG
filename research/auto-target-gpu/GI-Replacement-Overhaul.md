# GI replacement overhaul — implementation baseline and acceptance gates

Baseline: R465, commit 500c555824b24967b981d988e11309a853569868.
User mandate: general solution across the game, FPS included in the architecture,
four-ray performance target, ten-ray comparison, one distribution package.
This is an implementation audit and work specification, not a completed overhaul.

## Verified ownership boundary

- `gi_live_toggle.h::Dispatch`: an accepted automatic ray dispatch returns early;
  the original ray dispatch is not additionally issued through this route.
- `native_lighting_live_runtime.h::ComputeDispatchHook`: an accepted automatic
  evaluator prevents the original matching compute dispatch through this route.
- `gi_auto_raygen.hlsl`: owns receiver decoding and sampling but uses the existing
  scene acceleration structure and native hit/miss exports in the same state.
- `gi_auto_lighting.hlsl`: owns the ray loop and weighted accumulation, but calls
  the generated native secondary evaluator for material/light evaluation.
- `d3d12_auto_invocation.h`: retains native per-ray hit textures; adds training,
  four guide-building dispatches, per-invocation immutable output allocation and
  host lifetime tracking. Training/cell/ray-record scratch is already recycled.
- RR is downstream. Current timestamp scopes cover ray dispatch, evaluator and
  guide construction, not the full frame, native scene preparation or RR cost.

Therefore duplicate complete native GI execution is not an established cause of
current FPS loss. Prove actual pass execution and whole-frame costs before removal.
Do not remove unrelated passes based on names or assumed redundancy.

## Audit gate before algorithm replacement

Trace producer origin/direction and native hit-record encoding through decoded hit,
material, raw RGB, sample probability, final weight and RR input. The current
pixel audit reports many direction mismatch flags. Their tolerance/coordinate
semantics have not been explained sufficiently to classify them as harmless or
as a rendering defect. This is an open correctness item, not evidence that a larger
guide cache alone will solve the issue. Capture enough per-ray provenance to
separate reconstruction error, native hit representation and actual pairing error.
Keep this instrumentation opt-in, with normal-play overhead measured separately.

Use an independent high-sample reference and known analytical fixtures. Existing
paired-sampler parity demonstrates consistency with the previous sampler, not
correctness of every native material, hit path or RR input.

## Replacement architecture requirements

1. Retain native geometry and acceleration structures initially. Preserve hit
   material/texture access until replacement evaluation is independently verified.
2. Replace the two globally ranked guides with bounded receiver-region guidance.
   Candidate relevance must consider receiver geometry and expected contribution;
   global peak radiance alone is insufficient. Avoid hardcoded scene positions.
3. Separate proposal history from radiance history. Bound history age and invalidate
   on changed scene/device state; preserve exploration and visibility evaluation.
   A stable proposal must not reuse stale lighting or create ghosting.
4. Freeze identical proposals and selection probabilities for tracing and shading.
   Maintain full sampling support and correct weights as candidates change. Four
   and ten rays must execute their actual budgets, with explicit mode logging.
5. Simplify or replace retained native secondary evaluation only after identifying
   its measured cost and reproducing material, shadow, exposure and light behavior.
6. Eliminate an obsolete stage when its replacement is enabled. Document every
   retained stage, allocation, transition and synchronization dependency.

The precise cache shape and candidate count are not chosen yet. Compare bounded
regional candidate designs against the current baseline before committing to a
costly global search per pixel or increasing ray counts to hide selection failures.

## Whole-frame performance acceptance

Measure matched stationary/moving scenes with FG off and without capture readback.
Report CPU preparation, complete GPU frame, GI trace, hit evaluation, guide building,
RR, allocation rate and synchronization. Do not add overlapping GPU scope durations
and call the result a frame-time saving. Use native GI, R465 four/ten, and replacement
four/ten at identical resolution and settings. Eventual 1–2 FPS overhead is a goal,
not a claim or a hard development cap. Do not normalize the current 10–15 FPS loss.

## Quality acceptance

Reuse existing captures, then cover bright small sources, diffuse light, dark rooms,
skin, hair, foliage, moving lights/characters, disocclusion and traversal. Require
retained illumination and detail, no new ghosting/boiling, and reduced temporal
outliers against reference. Selected strongest-spike logs are not a count of visible
artifacts or proof of whole-game correctness.

## Unified delivery

One ZIP and one DLL, containing both 4-ray and 10-ray launchers. R465 already builds
this combined artifact; stop splitting it through the variants helper. Existing
launchers pin their ray budget for the session, so the in-game sample menu does not
override them. Restart with the other launcher to compare. Live menu switching can
be enabled separately once transitions and any new history are tested.

## Current status

Combined R465 baseline packaged; no new visual fix or FPS gain claimed. Overhaul
implementation is not complete. First engineering gate is the producer/hit/evaluator
contract audit, followed by regional-guidance implementation and whole-frame costs.
