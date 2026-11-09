# R247 accepted baseline and follow-up validation

Preserve production commit 25fbd3f1e2359c0429190ed7b247f7bead726bc8.
Artifact: https://github.com/Frooglemcdoo/Control-FG/actions/runs/41205049671/artifacts/13044387458
Installed dxgi.dll SHA256: b5929301ce38f83eab21c0e2050dae8b8a5749fcdfd166a8a621b00a2727b06d
Settings: 4 ms, 16 scan partitions, existing R247 rendering settings.

The user reported stable performance in two runs, including combat with no
observed frame spikes or stuttering in the second. These are successful live
runs, not a controlled attribution of the entire FPS improvement to prewarming.

| Latest session PID | Accepted/completed | Failed | Final pending | Allocation max | Allocations >=16ms |
|---|---:|---:|---:|---:|---:|
| 2040 | 9490 | 0 | 0 | 11.922 ms | 0 |
| 30056 | 11074 | 0 | 0 | 10.989 ms | 0 |

Both initialized at Present 1. PID 2040 finished initialization before demand;
PID 30056 had 1214 pending requests when both lanes became ready. Both drained.
Sparse frame timing samples are not whole-run FPS averages or complete hitch
histories. The first run's cumulative queue latency did not improve over R245.

R248 is validation work only. It changes no production source, shader, launcher,
queue depth, streaming settings or runtime scheduling. Added tests cover:

- 32 newly created destination-resource cycles across the existing formats;
- releasing caller COM references during a pending Storage phase;
- rejecting Submit/Close while busy without abandoning the transfer;
- rejecting a partially prepared invalid batch, then accepting valid work;
- continued arena reuse without growth after prior high-water allocation;
- final mip bytes after turnover, using the existing 72-mip readback comparison;
- terminal native retirement: failure completion exactly once, no retry,
  balanced engine references and pending reservations, no successful latency entry.

These are deterministic transport and mocked native ownership checks. They do
not simulate forced scene destruction inside Control, a real device removal,
or engine cancellation. Do not describe them as full-game lifecycle validation.
No new installation or capture is requested for this validation-only change.
