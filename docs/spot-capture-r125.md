# R125: scoped fence capture

R124 aborted on any multiple-producer fence or repeated/decreasing value, including
fences unrelated to the shader-table copy. Its log did not distinguish those cases.
R125 accepts repeated values and isolates unsupported fences instead of canceling
the request. The R124 log cannot establish which branch triggered that abort.

## Test

Close Control to install beside Control_DX12.exe. Preserve NVIDIA runtimes and
ControlFGStreamline. Load the affected room with RT Contact Shadows on, RT samples 1,
FG and glass enhancements off. Select BOTH, play for 70 seconds, then SAVE LOG and
send Lighting-R125. No new screenshots are required. Selecting BOTH again rearms
capture without restarting the game.

## Changes

Repeated signals use the earliest sufficient signal's dependency clock. They cannot
import work submitted after that signal. Fence rewinds, multiple queue producers and
CPU signals mark only that fence unsupported. Waits on it add no cross-queue proof;
the CPU model continues processing same-queue order without adding GPU commands or
waits. This is not a claim that the native wait has completed.

Dependency clocks record fence provenance. If a fence becomes unsupported later,
imported proofs on affected queues and transitive signal snapshots are discarded;
local queue order is retained. Mirrored coverage that relied on the invalidated
proof is discarded. Independent queue clocks and unrelated coverage remain usable.
A later supported dependency can validate a fresh consumer in the same request.

The log records distinct isolation reasons, fence and queue identities, previous
producer, requested/previous/baseline values, affected proof counts and discarded
coverage. It also reports repeated signals, pending waits and whether dispatch data
was dependency-ordered. Logging is bounded.

Failed native calls, overlapping same-queue calls, conflicting unordered target
writes, unknown writes, replayed lists, limits and timeouts still stop capture.
Storage limits remain 64 MiB upload archive, 8 MiB mirror, 1,024 recording events,
4,096 queue operations, 32 queues and 128 fences. Unobserved queue implementations
and external CPU synchronization are not reconstructed. CPU submission evidence is
not GPU completion confirmation or actual intersected geometry.

## Validation

Windows tests cover repeated values without importing later work, unrelated fence
isolation, revocation of existing transitive proofs, independent recovery, CPU signal
forwarding, and exact diagnostic fields. Existing future-wait/native cross-queue,
concurrent recording, copy integrity, rearm/reset and bounds tests remain.
Render shaders, centered shadows, GI filtering, material guides, glass and the
60% clamp are unchanged.

API references:
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-wait
