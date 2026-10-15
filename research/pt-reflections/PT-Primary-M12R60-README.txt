M12R60 - eliminate duplicate plan validation work

R59 Quality: prepare 3.425 ms, plan validation 1.653 ms, scene preview 0.934 ms,
resource retention 0.298 ms and proof construction 0.317 ms. Lock waits negligible.

Plan preparation now validates each unique BLAS allocation once per immutable
scene. Topology still checks every instance. Vectors reserve expected capacity.
For paired ray records, full geometry/root conversion is performed on the first
record. Every second local root is still compared byte-for-byte to the same
binding; its independent native/private identifiers are checked before copying
the validated private root. Frozen geometry address and slice patches remain.
No cross-frame cache or stale scene reuse is introduced.

Regression compares records with full conversion, corrupts each consumed root
byte of each ray, checks invalid identifiers and alpha selection, and repeats
instances of one BLAS. Existing frozen geometry, allocation reuse, R16 padding,
scene ordering and resource lifetime tests remain.

Informational local benchmark: 4096 rows/8192 records, 1024 BLAS/4096 instances,
250 iterations. Preparation 1.480 -> 1.058 ms; output hash, token count and BLAS
versions identical. This is not an in-game frame-time or FPS prediction.

Close Control, extract beside dxgi.dll. Keep PT on and compare DLAA/Quality in
the same scene for around 30 seconds each. Close and run
Collect-PT-Primary-M12R60.cmd. Send its ZIP and average FPS. No enable script.
R58 flicker fix, R59 detailed timings, saved PT mode and shader binaries remain.
