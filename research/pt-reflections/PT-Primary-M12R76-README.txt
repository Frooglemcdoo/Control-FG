M12R76 - isolate rejected surface consumers from shared PT output

R75 still reports flicker/noise and native-light pop-in. Its logs repeatedly
enter surface_submission_changed. The implementation previously called
CancelFrame from that consumer check, disarming the producer and clearing the
radiance shared by every surface consumer and temporal history for the frame.
The logged count is draw-check failures, not a count of unique failed frames.

R76 gives each recording/frame consumer group a small GPU guard. A failed
consumer validation clears only its group's guard; other consumers keep their
PT output. The patched surface shader checks that guard before loading PT.
Producer/scene validation, wrong-queue handling, replay protection, and resource
retirement remain mandatory. Failure to clear a guard still fails closed.
No CPU-mapped flags are modified while the GPU could read them.

Guards are shared across draws within the same recording/frame, not across
recordings. Up to 64 idle guards are cached separately from the six large idle
outputs. Small guards cannot evict large output buffers on a size mismatch.
Native shaders/settings and PT-off behavior remain unchanged. No extra rays,
no lighting gain, and no new filter tuning. R75 split reconstruction is retained.

PT_PRIMARY_CONSUMER_REJECT records independent reasons: missing/invalid producer,
replayed recording, wrong queue, changed view, changed descriptors. This allows
remaining consumer fallbacks to be distinguished without cancelling the frame.

Validation includes actual queue-ordered guard enable/clear and a WARP draw
sequence where a rejected consumer selects native while another consumer still
reads the same unchanged PT buffer, plus existing PT/RT regression checks.
Target-game stability remains unverified. This fixes frame-wide cancellation
from consumer failures; it does not establish that every flicker source is gone.
Object motion, unsupported materials and missing-light sources remain unfinished.

Close Control and replace ALL package files. Preserve existing settings.
Do not rerun PT-Enable when already enabled. Collect results with
Collect-PT-Primary-M12R76.cmd after checking the same scene/camera movement.
