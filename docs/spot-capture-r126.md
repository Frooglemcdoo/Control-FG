# R126: capture starting boundary

R125's log shows a model cycle at capture startup: one queue waits for 6644 while
the other waits for 6641, but the first captured signal for the latter fence is
6644 behind the first wait. Signals queued before arming can still be incomplete
when the initial fence baseline is read. R125 incorrectly waited for the later
captured signal and spent its 64 MiB copy budget without replaying a table.

## Test

Close Control to install beside Control_DX12.exe. Preserve NVIDIA runtimes and
ControlFGStreamline. Load the affected room with RT Contact Shadows on, samples 1,
FG and glass enhancements off. Select BOTH, play for 70 seconds, then SAVE LOG and
send Lighting-R126. No screenshots are required. BOTH can rearm without a restart.

## Correction

For an unresolved initial wait below the first captured signal (or before any
signal is captured), the observer can sample the native fence CompletedValue.
It progresses only if that value actually satisfies the wait. It imports no
cross-queue dependency from the missing signal and discards all partial table
coverage. Fresh observed copies and supported ordering must validate the table.
The fence baseline is not advanced to excuse subsequent captured dependencies.

This reads native completion without waiting for it. Polling is throttled per
fence to at most once per 2 ms during pending queue processing. At most 128 fence
references are retained, released on rearm. The diagnostic adds no GPU commands,
GPU waits, CPU sleeps, worker threads or larger copy budget. The new boundary log
includes requested/completed/baseline/first-signal values and explicitly states
that no dependency was imported and coverage was discarded.

R125 repeated-value handling, unsupported-fence isolation and proof revocation
remain. Unsupported dependencies, unobserved queue implementations and external
CPU synchronization are not reconstructed. Successful validation still means
CPU-observed table bytes and submission ordering, not actual intersected geometry
or GPU completion confirmation for the captured table.

## Validation

The Windows WARP test queues native signals 6641 through 6643 before arming, held
behind a test-only gate so CompletedValue remains 6640. It then submits the exact
two-queue 6641/6644 cycle, verifies there is no premature model release, opens the
gate and checks fresh table validation and unchanged native GPU copy bytes.
Existing cross-queue, scoped-fence, concurrent-publication, rearm and bounds tests
remain. Rendering shaders, centered shadows, GI filtering, materials, glass and
60% clamp are unchanged.

API reference:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-getcompletedvalue
