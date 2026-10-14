# R42 evidence and R43 descriptor synchronization

The supplied R42 collector identifies commit
3e34aa5c9c4a7f5988f4d7deb3e4c885ddb0c585, normal startup capture, PT disabled,
no submitted primary PT dispatch and no primary preparation samples. Render
resolution changes from 3840x2160 to 2560x1440. The final counters show:

| Sampled category | Samples | Inclusive sampled wall time (ms) |
|---|---:|---:|
| descriptor_untracked | 1 | 0.0019 |
| descriptor_wait | 107378 | 1019.7805 |
| descriptor_native | 107378 | 17.3419 |
| descriptor_observer | 107378 | 219.7825 |

These are cumulative across threads, not per-frame critical-path durations.
R42 did not materially exercise its bypass. The existing fixed-table mirror
covers the destinations used by the game, so rejecting only unrelated copies
cannot address the dominant serialization seen in the R41 call stacks.

## Concurrent path

R43 retains all mirrors. A copy to a retained mirrored slot can share ownership
of the descriptor gate if neither source nor destination has ever carried
tracked sparse metadata. Eligibility is rechecked after acquiring the gate.
The native API and private mirror copy both finish before releasing shared
ownership. No map, source-token cache, AS index or UAV-format metadata changes
on this path. Conflicting native accesses must already be serialized by the
application under D3D12's descriptor-copy contract.

All operations which can change sparse metadata, heaps, or multiple slots use
exclusive ownership. Snapshot reads, refreshes and validation also acquire
exclusive ownership, so they cannot observe a native write without its mirror.
Recursive exclusive calls remain supported. Shared-to-exclusive upgrades are
not supported or used: forwarded calls mute observers, and legacy glass capture
runs after shared ownership ends. The gate is a single process-wide instance.

Heap bounds/pointers are published with release/acquire ordering and retained
for the Bank lifetime. Seen flags are bytes rather than packed bits, allowing
independent adjacent-slot writes. Version and statistics counters are atomic;
version exhaustion rejects snapshots. Existing source-token deduplication
remains only on the exclusive path; parallel copies capture the current source
value every time. This deliberately avoids publishing a token from a shared
transaction and can increase native mirror-copy counts while reducing locking.

The Selected/Signalling callbacks inspect their pending state under the existing
primary mutex before acquiring descriptor ownership. Empty callbacks return;
callbacks with retained PT work still perform their original admission logic.
No PT-off capture, geometry ownership, shader, queue ordering or lighting policy
is removed.

## Validation

The portable test exercises eight simultaneous shared owners, snapshot exclusion,
recursive exclusive locking and the existing randomized metadata reference.
The WARP test exercises 16000 adjacent-slot native/mirror copies with muted
reentrant callbacks, then checks complete immutable snapshots and the original
GPU readback. Cached-source copies must reject the parallel path without
forwarding, so fallback invokes the native API exactly once.

In-game performance is not yet verified. Test PT off, DLAA versus Quality,
without WPR. The three descriptor_parallel_* counters distinguish actual
concurrent-path use from the R42 untracked path. Retain conservative fallbacks;
do not infer success from the build or counter names alone.
