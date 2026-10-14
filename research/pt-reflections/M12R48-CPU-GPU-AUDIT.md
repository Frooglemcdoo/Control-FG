# R48 capture-path investigation

Matched seven-second CPU windows: capture-enabled/PT-off main thread running
6615.9798 ms; startup-bypass main thread running 6576.7642 ms. Similar total
CPU utilization does not resolve the throughput difference. Slow trace has
PT descriptor/capture paths and renderer cleanup/yield activity; this does not
establish the entire critical path or rule out GPU cost. Inclusive sampled
stacks overlap and must not be summed into per-frame timings. Folded generic
PDB symbols are not proof of a PT function executing in the bypass run.

Source audit at feb235ebd35e4b9748fca29682ea06f2ed917a08:
- pt_scene_capture_runtime.h Capture::Build calls FreezeGeometry while
  preserveGeometry is true, independent of renderingRequested. PT-off updates
  remain preserved to avoid stale deformations when objects stop updating.
- pt_geometry_snapshot.h records CopyBufferRegion and source/destination
  transitions in the intercepted game list. This adds GPU work even while
  PT rendering is disabled. The current copy_bytes metric is retained storage,
  not bytes transferred per frame, and cannot quantify GPU time/bandwidth.
- pt_scene_queue_runtime.h holds one recursive queueMutex across capture
  bookkeeping and native Execute/Signal/Wait calls. This is a possible CPU
  serialization cost, not evidence of a measured GPU stall.
- R48 retirement fence reuse works but did not restore user-reported scaling.

Next gate: matched R48 CPU+GPU recordings, same scene/settings/Quality,
PT-enabled startup with PT off versus RT-only startup. Inspect event loss,
actual frame cadence where exposed, GPU execution occupancy and submission
bubbles before choosing an optimization. WPR GPU events alone may not expose
individual command/barrier costs; do not claim that they do.
No renderer implementation changes are included in this diagnostic package.

## Matched GPU recordings (14 October 2026)

Original split archives reassembled and SHA256 verified; inner ETL hashes also
verified. Both R48, PT off, Quality labels, zero reported dropped events.
Seven-second interior windows exclude start and save/rundown.

| Metric | Capture enabled | Startup bypass |
|---|---:|---:|
| DXGI presents in window | 454 | 641 |
| Present cadence FPS | 64.84 | 91.61 |
| Mean interval ms | 15.422 | 10.916 |
| Main-thread on-CPU ms | 6652.4 | 6762.9 |
| HAGS queue-derived work union ms | 3933.2 | 5245.5 |
| HAGS queue-derived occupancy | 56.2% | 74.9% |

GPU scheduling is hardware accelerated. Work estimates reconstruct ordered
QueuePacket start/stop spans, exclude waits, and union overlapping queues using
the approach documented by Intel PresentMon GpuTrace.cpp. All game queues map
to the captured game's device/context; zero unmatched/out-of-order nonwait
stops within the measured windows. These are queue-derived estimates, NOT
hardware timestamp measurements of individual shaders/copies/barriers.
Trace overhead reduces FPS relative to the user's untraced results. No exact
untraced cost is claimed. The observed gap is dominated by time outside the
estimated GPU-work union; heavier GPU execution alone is not supported.

Queue lock wait increases by 8.40 ms across 6.875 seconds, while queue observer
wall time grows by 377.02 ms. Neither number is a whole-frame CPU cost.
GPU-profile capture-enabled sampled-IP data is available but its direct stack
joins are incomplete (862/25379); do not compare inclusive stacks from that
capture to bypass. The earlier CPU-only traces had near-complete stack joins.

R49 targets an independently demonstrated CPU issue: CbvHook takes the global
exclusive descriptor gate for untracked CPU CBV writes. Such writes create no
new AS or typed-UAV metadata. The existing monotonic filter and mirror bounds
can skip observation while still forwarding native creation and legacy capture.
Tracked addresses retain source-token invalidation, UAV/AS clearing and mirror
updates. Geometry preservation, retirement and all GPU shaders remain intact.
