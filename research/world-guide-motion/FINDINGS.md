# Automatic world-target guide: motion and flat-field admission test

Status: candidate rejected for gameplay integration in its current form.
No rendering changes, release package, or FPS claim.

This is a fresh-visibility synthetic CPU experiment, not a replay of Control or
an actual time series. Each case represents a changed scene against a frozen
previous target. Independent current rays intersect an analytic bright sphere,
with a moving opaque half-plane blocker and an independent random range draw.
The test evaluates RGB expectation against two independent scrambled-Sobol
references, nonnegative output, constant-field preservation and sampling variance.

Each case uses 180,000 groups of ten rays. Methods share random draws for paired
comparison. The original-game ten-ray stratification and RR are not modeled.
Light changes, blocker entry/motion/removal, receiver turn/translation, an old
bound surrounding the receiver, black output and invalid-guide fallback are covered.

## Tested estimators

1. Ten independent cosine draws.
2. Ten draws from 90% cosine + 10% target-cone density q, each weighted p/q.
3. One independent cosine pilot C0 plus nine q draws, resolved as

   R = (1 - sum(w_i)/10) C0 + sum(w_i C_i)/10, i=1..9, w_i=p/q.

For frozen proposals independent of the pilot, this preserves expectation.
For alpha <= 1/10, w <= 10/9, so all coefficients are nonnegative. Constant RGB
is exactly reproduced since coefficients sum to one. Every fresh contribution,
including miss/background lighting, is included. No stored radiance is reused.
The pilot is one of the ten rays, not an eleventh ray.

This does NOT permit choosing the guide or pilot after observing current samples,
sharing pilot randomness with the other draws, or copying weights from an unrelated
producer distribution. The native range and strata must be audited separately.

## Results and rejection

All RGB expectation, finite/nonnegative, black-output and constant-preservation
assertions passed. Invalid-guide fallback matches the native comparator to 1e-12
with common random inputs. Passing these checks is necessary but not sufficient.

| Case | Plain-guide variance / native | Pilot-guide variance / native |
| --- | ---: | ---: |
| Stationary bright patch | 0.0810 | 0.2705 |
| Light moves, old target retained | 1.1094 | 1.1949 |
| Blocker enters | 0.0861 | 0.2558 |
| Receiver turns | 0.0578 | 0.2608 |
| Receiver inside old target bound | 1.0880 | 1.1221 |

When the light switches off or the receiver turns completely away, the ordinary
guide introduces approximately 3.6e-5 / 3.9e-5 luminance variance to a constant
field where native variance is zero. The pilot variant avoids that regression,
but increases variance by about 19.5% in the stale moved-light case. It is not a
universal solution. An earlier unpaired run gave roughly 17%; the paired run in
results.json is authoritative.

These numbers are synthetic sampling variance, not measured motion boiling,
energy correctness in Control, GPU timings, or a predicted image improvement.

## Required next design change

Do not wire this previous-target model into the accepted renderer as a fix.
The simple world-space target idea needs a concrete freshness/validity source:
current illumination and secondary geometry must keep targets useful, with
fallback chosen before drawing rays. Stable scene/object identity is not supplied
by frame-local material IDs. Camera-cut invalidation alone is insufficient.

A next-frame target list with no moved-light or moved-secondary handling repeats
the failure already demonstrated here. A same-frame pilot/training stage would
need a separate dispatch and correctly paired producer/evaluator bindings; its
cost and native-work suppression must be designed before claiming a low-cost fix.
Do not use current-sample brightness to retroactively disable weighting.

Existing frozen-guide/runtime machinery is experimental and has previous cost
regressions. Its existence is not permission to restore that rendering path.

## Follow-up: rebuilding targets from fresh samples

`learning_sequence.py` implements a separate one-frame target-learning experiment
using the positive pilot resolve. 2,048 receivers each execute ten fresh rays per
frame for 120 synthetic frames. The next target is learned only from actual bright
hit positions and density-corrected training energy. Empty/insufficient training
expires the old target. Scene state and the numerical reference are not supplied
to the guide builder. Current visibility and radiance are always re-evaluated.

It covers continuous receiver translation/rotation, a moving bright secondary
object, abrupt source relocation, blockers and source removal. Overall squared
error is 0.3337 times the independent-cosine comparator; steady-section ratio is
0.2581. Light-off output remains equal to native to 1e-12. Abrupt source relocation
still increases error on the first stale frame (combined two relocation frames:
1.3666 times native); the next target is learned from the new visible hits.
The first moving-blocker interval almost entirely removes the bright object and
falls back to native; its ratio of 1 is not evidence of better shadow-edge quality.

This advances the candidate beyond the deliberately frozen-target rejection above,
but is not a gameplay admission result. It has one analytic bright object, CPU
learning, a fixture brightness threshold of 1 and independent samples. Multiple
sources, material changes, native stratification/range semantics, GPU target build,
resource lifetimes and RR remain unvalidated. Do not extrapolate the 0.3337 number
to Control quality or performance.

## Implemented shared resolve and checks

`src/gi_reuse/pilot_guide_resolve.h` implements the nonnegative pilot resolve in
shared C++/HLSL source. It rejects incomplete batches, mismatched proposal versions,
nonfinite/negative RGB, invalid weights and unsupported mixture fractions. It
corrects only floating-point overshoot of the theoretical coefficient bound.
Invalid sampled results are not silently reinterpreted as unweighted native rays.

`tools/test-gi-pilot-guide-resolve.cpp` exhaustively enumerates a colored two-state
lighting distribution at 2, 4 and 10 rays, verifies RGB expectation, and checks
constant fields with adversarial zero/max weights through 64 rays plus invalid
inputs. Built and passed with g++ C++17, O2, Wall/Wextra/Werror.

`tools/gi-pilot-resolve-validation.hlsl` compiles the same shared implementation
with DXC cs_6_0 and warnings as errors. Compilation passed. No GPU execution,
renderer binding, or runtime use is claimed; the accepted game path is unchanged.
