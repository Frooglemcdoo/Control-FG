# R146 — independent ray/composition capture submissions

R145's user trace stopped approximately 1.2 seconds after Shift+F9 with
`submission_before_composition_signal`. The native renderer submitted the ray
capture list before composition was recorded. This is a capture scheduling bug,
not a traversal timeout or evidence about the shadow-loss cause.

R146 tracks the two recordings independently: command-list owner and generation,
actual submission queue, fence, and retained resources. The composition stage
can record on a later list/generation in the same verified spotlight scope and
present. Completion requires both fences; neither stage alone permits reading,
exporting or reusing capture storage. Separate readback buffers avoid concurrent
writes from separate queues into one diagnostic resource. Same-batch submission
remains supported. Native scheduling is preserved, with no added queue wait or
render-thread fence wait.

Descriptor observation stays active after the ray stage submits. The sampler
requires matching extent/frame and the verified composition PSO. Both generations
are logged; decoding still requires matching ray/composition pixels and selected
light. Replay, reset-before-submit, stale generation and missing composition
checks remain active. Rearming requires both recordings to have reset and both
fences to be complete. In-flight/failed owners are retained.

## Test

Close the game and install the package beside Control_DX12.exe. Use Default,
RT Contact Shadows on, samples 1, FG/glass enhancements off. Press Shift+F9,
walk into the affected room and back, and wait for completion (normally 25–40
seconds). Six samples, four-second minimum spacing and the overall 60-second
limit are unchanged. Plain F9 remains the stationary comparison suite.

Send lighting.log and manifest.txt from Shadow-R146. If larger files/video need
packaging, drag the completed folder onto Package-Lighting-Capture.cmd and send
all ZIPs from its SEND folder, including capture-manifest.zip. Each ZIP is below
200 MB; originals are retained. The helper uses installed Python 3.8+.

## Evidence and limits

The supplied R145 files identify the expected build and fail at sample 0 before
any completed composition export. They provide no new shadow measurements.
R144 had six valid samples with renumbered but persistent principal light geometry.
R146 preserves R145's read-only signal sampler and the accepted rendering shaders,
No History default, native point history, authored clipping and surface-normal
correction. This fixes the demonstrated capture failure; it does not claim to fix
the underlying disappearing shadows.

Tests cover early ray submission/completion/reset before composition recording,
separate queues, same-batch submission, either fence completing first, wrong queue,
stale generation, replay/discard, missing composition at scope end, and rearm only
after both fences. The real 4096-pixel sampler remains covered by WARP. In-game
validation of this scheduling correction remains pending.
