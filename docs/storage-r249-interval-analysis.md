# R247 texture latency: interval analysis

Analysis of the three accepted R247 runs, using differences between successive
cumulative reports. Production R247 remains unchanged. The standalone analyzer
is tools/diagnostics/analyze-storage-latency.py; it accepts logs or compact ZIPs
and selects the latest probe session in each ZIP. Three local unit tests cover
baseline/delta arithmetic, unchanged/reset counters, and ZIP session selection.

| Session PID | Queue intervals with new samples | Intervals with mean below 10 ms | Later intervals adding waits >=100 ms |
|---|---:|---:|---:|
| 2040 | 20 | 16 | 1 |
| 30056 | 23 | 22 | 1 |
| 34076 | 58 | 55 | 2 |

The below-10ms counts include the rounding-error upper bound. A low interval
mean does not imply every request was fast. The first retained snapshot is
excluded from interval counts, and can contain substantial earlier loading.
The third log is compacted; startup markers were not retained.

For PID 34076, the two later queue windows adding >=100ms waits ended at Present
2940 (2627 requests, approximately 152.00ms mean, 1930 long waits) and Present
6780 (654 requests, approximately 41.42ms mean, 78 long waits). Together they
account for all 2008 added >=100ms waits after the first retained snapshot;
1214 earlier long waits are already included in that initial snapshot.
The final cumulative count is 3222. The two corresponding request-to-handoff
window means were approximately 156.20ms and 50.20ms respectively.

This supports burst-concentrated queue delay, not a persistent queue backlog.
It does not identify those windows as loading screens, traversal, or combat.
Five-second status snapshots with pending=0 also do not prove the queue stayed
empty between snapshots. Sample populations can differ at report boundaries.

Keep the successful 4ms/16-partition, two-lane implementation. Do not increase
queue concurrency or scan budgets based on cumulative means alone. These logs
do not measure native demand selection before RequestHook, deferred renderer
publication after finishLoadLevel, or the first visible draw using a mip.
Further work on visible texture arrival should establish those boundaries,
particularly request demand and publication, rather than assume disk or GPU
copy time dominates. No new game install or user recapture is required for this
analysis. No frame-rate gain or texture-arrival fix is claimed.
