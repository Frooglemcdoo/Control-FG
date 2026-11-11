# Sharing admission decision — 2026-11-11

Decision: do not integrate the current tile-sharing candidate into the game.
The GPU plumbing passed, but the quality gate has not. R423 remains unchanged.
This rejects the two tested admission heuristics, not all possible path guiding.

## What the stronger test changes

The earlier toy averaged variance across a tile. That can hide regressions on
a small differently lit region—the kind of local failure we must avoid while
preserving the accepted plant/desk/projector image. This is an adversarial
analogy, not attribution of any captured Jesse pixel.

`research/directional-guiding/sharing_admission_trial.py` uses 128 receivers,
64 discrete directions, a 0.125 guide fraction, 8192 pilot sets, and seed 427.
It assumes identical geometry/material while illumination differs, so normal,
depth and material grouping alone cannot distinguish these synthetic receivers.
The native comparator is two independent uniform draws, not Control stratification.

Three policies use the same pilot sets:

- Ungated sharing.
- Group evidence: at least three strong pilot observations in the dominant
  angular cell, which must contain at least half the observed energy. Strong
  means at least a quarter of the group's largest pilot contribution.
- Receiver evidence: the group rule plus the receiver's own pilot must be strong
  and fall in that dominant cell.

These rules alter only proposal choice; they do not clamp, average or discard
rendered RGB. The first sample remains unmodified. All conditional-mean checks
pass, but correct mean does not imply lower variance.

## Results that prevent promotion

Ratios below 1 reduce per-receiver variance; above 1 increase it.

| Case | Ungated | Group evidence | Receiver evidence |
| --- | ---: | ---: | ---: |
| Coherent group | 0.619007 | 0.851442 | 0.995743 |
| 127 majority receivers | 0.620168 | 0.853977 | 0.995788 |
| One differently lit receiver | 1.054417 | 1.021380 | 1.000000 |

The majority improves while the minority regresses. A whole-group quality
metric would conceal that. The stricter per-receiver rule avoids this case but
admits only about 0.945% of receivers in the coherent trial; its variance benefit
is about 0.426%, not a compelling basis for extra ray/shading scheduling.

In a group with eight constant-lighting receivers and 120 directional-lighting
receivers, group evidence adds variance 2.27283e-05 to the constant receivers,
whose native variance is zero. The receiver rule avoids that in this toy.
These results are not a proof that the receiver rule is universally safe.
Mixed-16-field cases are also retained in the JSON; do not omit their failures.

## Cost evidence reviewed

`historical-cost-evidence.json` preserves comparison rows, selected GPU stages
and ZIP hashes from all six locally available performance packages: R387,
R388, R389, R390, R391 and R414. Coverage failures/partial phases remain visible.

R391 native baseline sampled GI ray and evaluator spans are 0.2110 and 0.2529 ms;
RR evaluation is 3.1191 ms. R414 native baseline has 0.2600, 0.2615 and 3.0892 ms
respectively. These are historical sampled stage spans, not total GPU frame
times, and must not be summed indiscriminately or translated into an FPS promise.
R414's later return phases are incomplete; they are not a complete A/B result.

None measures the new split schedule. Keeping two rays does not remove its
extra dependency, repeated receiver setup, learning, publication and resolve
cost. The present evidence supplies no measured overhead or FPS gain for it.
Do not request a user performance run for a candidate that has not passed its
quality gate, or ship additional passes solely because the compute test passes.

## Next implementation constraint

Do not spend the next integration step wiring this tile heuristic into live
rendering. A replacement needs receiver-specific evidence about useful lighting
directions, not just same-material neighbors or a group-average benefit.
Any next proposal must first pass these minority/constant-field cases, preserve
native sample count and fresh visibility, and expose a credible cost schedule.
Current captures do not contain per-direction lighting fields that would certify
that evidence. No new generic capture or stationary test is requested here.
