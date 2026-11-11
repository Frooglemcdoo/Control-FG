# Current-frame directional proposal screening

Research implementation after the R424 source attribution. No runtime change,
new game package, measured FPS improvement or demonstrated visual fix.

## New work

`research/directional-guiding/captured_trial.py` now evaluates frozen proposals
against held-out samples from both actual captures. This is a different check
from the earlier synthetic, previous-frame tile guide that failed moving lights.
The old history guide remains rejected.

For native density p, candidate q and native sample contribution C=f/p,
the second moment under q is E_p[C^2 p/q]. Therefore captured native rays can
screen a proposal without pretending to render new rays. A correctly weighted
fresh estimator f/q preserves the integral, so second-moment changes correspond
to variance changes in expectation for a fixed receiver and unchanged scene.
One sample per varying receiver does not establish the true variance or mean
of each pixel. The reported ratios are empirical aggregate moment estimates,
not rendered noise percentages, convergence guarantees or FPS predictions.

The guide is a positive, normalized equal-area sphere histogram mixed with
at least 50% current-receiver cosine density. All incoming GI, including
miss/volume contributions, receives the same proposal correction. Visibility
and randomized ray range would still have to be freshly traced; this analysis
does not replace endpoints or reuse visibility.

## Preventing an optimistic result

- Two disjoint folds: only the other half trains the evaluated proposal.
- Both random pixel splits (four seeds) and contiguous 32-pixel block splits.
- 4x4, 8x8 and 16x16 directional grids; all results retained, not just the best.
- Whole-region, normal-octant and 0.25-world-unit-cell plus normal-octant groups.
- Fewer than 256 valid training rays in a group means native sampling fallback.
- Misses remain in learning and evaluation. They are not discarded as outliers.
- Endpoint directions shorter than 0.05 native units are uncertain; their
  p/q contribution is bounded conservatively between zero and 1/(1-alpha).
  Native fallback always has ratio one. These are geometry-uncertainty bounds,
  NOT statistical confidence intervals or bounds on the true scene variance.
- No raw per-frame material ID becomes a persistent key or character identity.

Training and evaluating on the same rays would favor overfitted proposals.
Changing ray directions and merely multiplying the old displayed pixels by
p/q would also be wrong. Neither operation is a rendering implementation here.

## Selected fixed candidate, including its weakness

For the 8x8 world+normal guide, 12.5% guiding fraction and contiguous block split:

| Captured region | Whole-region estimated second-moment upper ratio, stage order A / B | Guided miss-lighting second-moment ratio |
|---|---|---|
| Hair | approximately 0.811 / 0.805 | approximately 1.029 |
| Jaw/neck | approximately 0.967 / 0.924 | approximately 1.034 |

Each stage order leaves the training half's original native sample unchanged
and guides only the other half. There is no hidden extra ray in this model.
This improves the empirical aggregate more for hair than for the jaw. Miss
lighting becomes slightly noisier by this metric. Stronger guiding increases
that penalty. A whole-region guide can also mask local degradation; normal and
world-position conditioning helps, but does not prove robustness elsewhere.

The available data does NOT include the accepted projector/desk/plant scenes
under this new sampler, repeated source frames, new occlusion tests or GPU
timing. Promoting the best table entry to a user build would be premature.

## Direction reconstruction sensitivity

The recorder contains evaluator origins and quantized secondary endpoints,
not direct ray-generation direction/PDF records. A single-ray seed hypothesis
using source frame token+1 and batch0 closely matches recorded directions:
median angular difference about 0.0206 degrees for hair and 0.0561 for jaw,
versus over 55 degrees for the next-best tested offset/batch. The three-unit
randomized range hypothesis also matches miss distances within about 0.0033
native units. These are inferred constants, not exported raygen constants.

Repeating the fixed candidate using regenerated directions gives whole-region
ratios about 0.812/0.807 for hair and 0.959/0.908 for jaw, with miss ratios about
1.029 and 1.042. The broad conclusion survives this sensitivity check. This
does not certify exact per-ray PDFs or turn a one-frame screen into a motion test.

The regenerated hash was checked bit-exactly against the actual generated
Owned sampling IR for both native variants, 4,096 frames, one ray, fixed pixel
and batch. Discrete analytic fixtures additionally check second moments, energy,
full support, stale-guide penalties and held-out data isolation.

## Runtime audit: remaining implementation boundary

`gi_live_toggle::Dispatch` currently submits the complete native-budget ray
dispatch. The separated GI evaluator runs later with its own native compute
bindings. A guide learned from this frame's lighting is unavailable when the
current full ray dispatch begins.

A genuine same-frame design would need:

1. Trace and shade a disjoint training half exactly once.
2. Build/freeze the guide, with resource transitions and collision-safe world
   cell ownership. No surface or material history is assumed.
3. Trace the remaining half using fresh visibility and the frozen proposal;
   evaluate with its exact PDF, including miss lighting and unchanged range law.
4. Publish only a complete GI frame, restore all native compute/ray bindings,
   and fall back before sampling or with explicitly matched native PDFs.

Running a full native frame then replacing half would instead cost 1.5 times
the ray count and is rejected. A single dispatch cannot safely communicate an
arbitrary global guide across thread groups without a separate synchronization
boundary. The existing ray dispatch hook alone is insufficient to establish
the required producer/evaluator ordering and resource lifetime.

Thus this is a candidate source-sampling architecture, not a cheap shader tweak.
Its extra dispatches, guide storage/atomics and binding restoration need an
explicit budget and validated scheduling before a gameplay build. Do not
re-enable old biased endpoint reuse, RELAX or R422 while implementing it.

## Reproduce

First run the R424 capture analysis to create trace arrays. Then:

    python research/directional-guiding/captured_trial.py <capture_root> --output <scratch-output.json>

Run without arguments for the generated-IR and analytic tests only.
Compact measured results are in `research/directional-guiding/r424-screen-summary.json`.
The full report is deterministic and regenerable from the saved captures.

Mathematical reference (importance sampling and efficiency):
https://pbr-book.org/4ed/Monte_Carlo_Integration/Improving_Efficiency
