# M12R5 corrections

R4 run 20261012-224728: 7215 failed copies, zero allocated copy buffers and zero
primary PT submission/coverage. Last CPU record: build_total_ms=1277,
checkpoint_total_ms=26201, checkpoint_max_ms=63, checkpoints=956.

## Cleanup

Old Compact loops every retained root for every target/lane. With 1500 static
BLAS writes and 3000 pending signals this took 134.925 ms in the local optimized
C++ fixture. Invert the query to find thresholds within each write's predecessor
and successor intervals: 2.719 ms in the same fixture, same ordering decisions.
These are CPU measurements, not RTX 5090 frame times. Existing randomized
compacted/uncompacted decisions, future writes and 20000 busy updates still pass.
Checkpoint batching rises from 32 to 256 native queue operations.

## Buffer defaults

Hash-gated native d3d helper 0x28150 returns 0x41 for vertices and 0x42 for
indices when native ray tracing is enabled. Those values are creation/default
read masks, not permanent recording states. Captured overview resource 5608
(index source at GlobalId 1141, compute list 5832) is a DEFAULT buffer initialized
in COMMON; unlike dynamic position 3163 it has no local transition at that build.
D3D12 buffers decay after ExecuteCommandLists. Normalize only native read defaults
when no transition has been recorded on the list; never normalize UAV/COPY_DEST
or an explicit/pending transition. The new WARP case uploads and completes a
buffer, resets the compute list, then copies it with the unchanged 0x42 native
wrapper. Existing byte preservation, lifetime and overwrite checks still apply.

References:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html

## View and OFF-state work

R4's failureOffset was the last provider-read offset, not the actual mismatch.
Now log the actual mismatching field and its bound/provider words. Retain bound
pass resolution; environment derives inverse extent from its bound screen size.
Compare affine xyz lanes consumed by the private/native paths; ignored .w lanes
are not camera identity. Projection, frame and consumed camera changes still
reject. Thread-local providers are authenticated and sampled twice as before.

OFF dynamic updates become tombstones without copy commands. Static initial
builds remain captured for later activation. This is reduced background work,
not a claim of zero overhead. Surface setup does no camera mapping/descriptor
allocation when there is no recorded producer. No native rendering shader changed.

Windows build and actual game results must be reported separately. No in-game
coverage or FPS improvement is claimed by these local tests.
