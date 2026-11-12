CONTROL RT OWNERSHIP R433

Extract this folder anywhere, close Control, and run Run-RT-Ownership-R433.cmd.
Return RT-Ownership-R433.txt whether it passes or fails.

This tests new submission-ordered PTLAS ownership on your NVIDIA GPU. The older
R392/R394 tests waited for GPU completion between updates; their existing passing
results cannot validate this new path. This test submits 32 updates while the GPU
queue is deliberately held, then releases it and checks all 256 ray results.
It includes unchanged scenes, movement, mask changes, removal, empty scenes,
slot reuse, BLAS-address changes and discarded recordings in one run.

This is standalone, not a mod installation or a new capture. It does not rerun
the old captured-descriptor suite. No native TLAS is built, no geometry is copied
per update, and there is no CPU/GPU wait between updates. A final wait retires all
resources safely. The geometry is two owned test triangles, not Control geometry.

No FPS or noise improvement is claimed. Game producer retirement, complete native
consumer remapping and dynamic material ownership are not enabled by this test.
