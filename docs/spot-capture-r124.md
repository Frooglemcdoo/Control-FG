# R124: fence-ordered native table capture

R123 stopped at a second relevant queue. R124 observes native queue ExecuteCommandLists,
Signal and Wait calls and builds a bounded per-queue timeline. Recording and submission
remain independent. A consumer wait submitted before its producer signal stays pending
in the CPU model until that signal is observed and succeeds. No GPU waits or commands
are added by the diagnostic.

## Test

Close Control to install beside Control_DX12.exe. Preserve NVIDIA runtimes and
ControlFGStreamline. Load an affected area with native RT Contact Shadows on,
RT samples 1, FG and glass enhancements off. Select BOTH after loading, play for
70 seconds, then SAVE LOG and send the Lighting-R124 log. Selecting BOTH again
starts a fresh capture without restarting the game.

## Ordering and limits

Each queue carries a dependency clock. Successful fence signals publish that clock;
waits import it only after a sufficient signal is observed. Transitive dependencies
are preserved. Unordered consumer dispatches are skipped rather than validated or
aborting the entire request. Conflicting unordered writes stop capture explicitly.
An already completed fence value observed before its first captured operation is
an external boundary, not evidence that any captured producer has completed.

The model requires one producer queue and increasing values for each observed fence.
Fence rewinds, multiple producers, CPU fence signals, failed native queue calls,
overlapping calls on the same queue, unsupported writes, replayed lists and exhausted
budgets stop capture explicitly. Different queues can submit concurrently. Unresolved
waits remain pending until their signal arrives or the capture expires. External or
unobserved queue implementations and CPU synchronization are not reconstructed.

Storage is bounded to 64 MiB of sampled upload bytes, an 8 MiB mirror, 1,024 recording
events, 4,096 queue operations, 32 queues and 128 fences. The capture is idle before
BOTH is selected and stops after validation or a limit. This validates CPU-observed
submission dependencies and sampled shader-table bytes, not GPU completion or actual
hit geometry. Render shaders, centered shadows, GI filtering, material guides, glass
and the 60% clamp are unchanged.

## Validation

Windows WARP tests exercise native Signal/Wait forwarding with a consumer submitted
before its producer, exact and greater fence values, three-queue dependency chains,
concurrent queue calls, and rejection of failed or ambiguous signals. The existing
concurrent publication, reset/rearm, copy coverage and unchanged GPU byte tests remain.

API references:
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-wait
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
