# R122: requested submission-order table capture

R121 captured during level loading and exhausted its CPU budget before BOTH was
selected. Its single recording-list mirror could not relate copies to a dispatch
on another command-list generation. This build replaces that mirror with a bounded
copy/event journal processed in observed ExecuteCommandLists order.

## Test

Close Control to install beside Control_DX12.exe. Preserve NVIDIA runtimes and
ControlFGStreamline. Load an affected area with native RT Contact Shadows on,
RT samples 1, FG and glass enhancements off. Select BOTH after loading; this arms
capture. Play for 70 seconds, then SAVE LOG and send the Lighting-R122 log.
Selecting BOTH again starts a fresh capture without restarting the game.

## Evidence and limits

The journal stays idle until BOTH is selected. Each copy stores its command-list
reset generation, destination resource identity, extent, offset and stable CPU
upload bytes. Spotlight dispatch events store their table extent and native shader
identifiers. The actual submission array determines processing order, including
separate producer and consumer lists and successive submissions on the same queue.
A command-list reset cannot reuse an unsubmitted generation. Replays, lost events,
unsupported writes and cross-queue dependencies stop capture explicitly.

Cross-queue Signal/Wait dependencies are not reconstructed in this build: it reports
cross_queue_order_unverified instead of claiming those queues are ordered. Queue
hooks are installed using direct/compute/copy probe queues from the active device;
other queue implementations may remain unobserved. This remains CPU submission
ordering evidence, not GPU completion confirmation or intersected geometry.

Speculative upload-resource retention and startup table polling are disabled.
Storage is bounded to a 64 MiB copy archive, 8 MiB mirror and 1,024 events. Capture
ends on validation, a limit or timeout. No GPU commands, waits, render shader edits,
or descriptor changes are introduced. Shader, GI filter, material-guide, glass
and 60% clamp behavior is retained.

Windows validation exercises producer/consumer lists recorded in reverse CPU order
and submitted in the correct order, unchanged GPU/upload results, user-triggered
arming, rearming, reset/replay rejection, and rejection of unverified queue order.
