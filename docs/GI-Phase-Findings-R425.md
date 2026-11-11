# R425 result: separate recordings, same queue, distinct phase bindings

Input: `GI-Phase-R425-20261111-104355.zip`, Library
`libfile_2cf0c0d58c448191bea2870a46ec199b`.
Inner log SHA256:
`9238c40bbaf6e000e076438b00a438d409a80b27e3c77b6a66215cb3c4793b17`.
The checksum matches `collection.json`.

## Observations

- 24 consecutive observed engine frames, 2762 through 2785.
- Exactly one captured GI ray event and one GI6 evaluator event per frame.
- All 48 binding snapshots ready; each exposes root argument slots 0 through 7.
- Every ray/evaluator pair uses separate command lists and separate, nonoverlapping
  ExecuteCommandLists calls, with ray submission first on the same direct queue.
- No other captured call on that queue overlaps any of the 48 target calls.
- Root signature and both descriptor heap identities match within each pair.
- Root argument slots **0, 2, 3, 4, 5 differ in every pair**. Slot 0 is a table;
  slots 2 through 5 are CBV addresses. Slots 1, 6 and 7 match within each pair.
- Heap identities rotate across frames. Shared layout is not interchangeable data.
- Target submissions have all returned by event 2452.

The complete log has 8192 retained events, contiguous IDs, and one dropped event
at the fixed capacity boundary (8193), much later than the target submissions.
The last retained events observe engine frame 3026. Continued generic queue/fence
logging after the target window consumed the remaining capacity. This is an audit
scope issue, not evidence of a rendering failure. Do not request a rerun solely
because the global overflow flag is set.

The parser now reports target-prefix completeness separately from the global
warning, preserving both. Synthetic fixtures test late overflow, loss inside the
required prefix, missing binding records and missing Execute return records.
This does not prove that no later resubmission occurred after truncation.

## Consequence for implementation

The native ray list is already submitted before the evaluator event is recorded.
Do not assume that capturing both bindings at evaluator time allows modifying or
re-recording the already submitted ray list. Do not replay the ray work using the
evaluator's CBVs/table just because roots and heaps match.

A current-frame disjoint split must prepare the training subset before the first
ray submission and preserve the ray phase's own binding data and native SBT
ownership for the second subset. At the evaluator boundary it can shade training,
freeze the guide, trace the remaining subset with preserved ray bindings, and
shade that subset with evaluator bindings. Explicit resource transitions and
complete output publication are mandatory. This is a proposed integration route,
not an implemented or GPU-validated schedule.

If admission fails after tracing only the training subset, full native fallback
would trace that subset twice. The fallback must finish only the complementary
subset with native sampling, using the same random stream/ray budget. If safe
ownership for even that completion cannot be guaranteed up front, do not begin
the split. Preflight resource allocation and an always-available complement path
are prerequisites to suppression of any native work.

Existing `GINativeGIBindings` / `GINativeSceneOwners` have writer handoff and CPU
source descriptor ownership machinery, but belong to the legacy owned pipeline.
Their existence does not establish that the current single-producer path already
retains those resources, nor justify enabling the full legacy scene observer and
its prior performance cost. Scope any reuse to the exact current GI invocation.

## Limits

This is CPU recording/submission evidence, not GPU completion, barrier correctness,
SBT writer lifetime, descriptor-resource lifetime or temporal-history correctness.
It does not identify a new cause of Jesse's noise and does not prove the sampler
will fix it. It resolves the specific scheduling/layout question blocking design.
No additional user capture is needed for that question. Continue implementation
from the existing owned-source findings and frozen-guide core; no new blur/history
parameter sweep or repeat hair/jaw capture is warranted.
