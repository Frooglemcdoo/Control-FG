# R427 v2: scene-ordering capture checkpoint complete

Input: GI-Scene-R427-v2-20261111-223937-75108-5c5ff0e3.zip.
Read all three files, verified JSONL hash against result.json, parsed all8192
contiguous event IDs. Startup confirms owned preparation, owned recording and
binding installed; install event25 revision2 confirms the corrected observer.

## Observed and checked

- All24 GI frames1405..1428 have one ray and one shading phase, each submitted
  exactly once in separate ordered Execute calls on the same graphics queue.
- All144 snapshots have valid mask63, no read faults, no tracked writer active,
  no serial change during a sample, and same-frame scene join complete.
- Within each frame, writer serial, scene identity/fence/value, job handle,
  native device fence/value and material-manager generation remain unchanged
  across all six recording/submission observation points.
- Ray and shading recording snapshots use matching descriptor heap identities,
  generation and retirement epoch. Their captured heaps match the actual phase
  binding snapshots at all48 recording points.
- For each frame, all three next-frame writer starts and the next scene-fence
  signal occur after the shading submission and its final metadata sample.
- Required phase/submission records end at2122; final scene/heap details end
  at2126. Global overflow occurs later at8193. It does not invalidate this
  complete target prefix. Do NOT request another capture merely for overflow.

## Important implementation constraint: current context is not recorded state

At76 of96 before/after submission observations (22 of24 frames), the native
thread-local current resource heap differs from the heap captured with the
submitted GI commands. It can carry an older material generation. This is not
proof of incorrect game bindings: recorded command lists retain their own heap
bindings, and the ray/shading snapshots match. The thread-local provider switches
between command contexts, as already visible in the state-pointer field.

Never resolve delayed GI descriptors from the submission thread's current
provider, or demand that provider equal the recorded one. Carry the exact
recording-time heap COM owner, native heap-record identity/generation/epoch,
root arguments and invocation through both phases. Validate the saved record
itself and the native retirement boundary. Do not substitute a newer heap or
use a later global current-heap lookup. Material generation changes between22
adjacent frame pairs, so history across frames must not reuse this snapshot.

## Decision and limits

This closes the requested empirical ordering/overlap checkpoint for owned
recording. No additional diagnostic capture is required for this question.
The next work is live host integration and an enabled visual comparison, not
another round of scene-ordering captures. No render behavior was changed in
this analysis turn; no new gameplay build has been produced.

The capture is observation, not a lock or ownership token. Scene/descriptor
admission remains false until the host enforces the exact native invocation,
recorded bindings, same-queue ordering and native fence lifetime established
by the code audits. COM retention alone does not freeze descriptor contents or
AS bytes. Do not turn on dispatch merely because24 samples were quiet. Keep
phase-A/phase-B private storage, copied SBT/constants and whole-invocation
retirement together; do not issue a full native retry after partial recording.

No noise reduction or FPS improvement has been demonstrated. No evidence here
identifies the queue's current-heap difference as the cause of the existing
jaw/hair noise; it is a constraint for the new replacement path.
