# R433 queued RT ownership

The existing R392/R394 GPU tests called `Flush` for every update, then advanced
the backend/planner. Their passing results do not exercise several ordered AS
updates awaiting execution. This leaves a narrow queued-NVAPI execution gap, not a missing generic ownership
foundation. The broader history audit below supersedes the initial request to
run this fixture before continuing source work.

`rt_ptlas_submission.h` now serializes planning against the last successfully
submitted revision. Prepare/record/close cannot advance that revision. Execute
admission binds one queue, private fence and increasing signal value, allocating
bookkeeping before execution. Successful Signal commits CPU history without a
GPU wait. Discard after Reset retains the previous submitted history, including
discarded initialization. Signal failure after execution quarantines the journal
and refuses further planning or retirement. Outstanding submissions are bounded
at 128. The host must separately retain resources, prevent list replay and
serialize journal calls; the class does not claim to supply these contracts.

`NvBackend::Submitted` advances its initialization state under this contract.
The previous serialized `Complete` path remains available for existing tests.
No game hook publishes a PTLAS backend, and no default rendering behavior changes.

The R433 hardware fixture owns its command lists/allocators, per-update uploads,
output/readback buffers, both BLAS dependencies and the PTLAS result/scratch.
It holds the GPU queue on a CPU-signalled gate, submits 32 updates, checks all 32
remain outstanding, then releases the gate and drains once. Independent analytic
hit results validate all 256 rays. Cases combine movement, masking, shrink/empty,
slot reuse, BLAS-address changes, unchanged no-build updates and three discarded
recordings. No conventional TLAS is built during this fixture. It uses synthetic
triangles, not captured Control meshes. Failed drain retains the entire state
through process teardown rather than releasing potentially in-flight storage.

Local warnings-as-errors, ASan/UBSan (leak detection disabled for this executor),
and the existing planner tests pass. Windows run 43288895526 / job 131429549447
compiled the queued executable and passed all 19 fixture tests. Source commit:
b6a73ff3c30f7955addf404c98ee0c6e8d48ef53. Artifact 13975278065 was downloaded
and checked against its digest, ZIP CRCs, source commit and every CI file hash.
Package verification is in `rt-queued-r433-package-verification.json`.
Delivered file: libfile_3449d998b28c8191b65729adcdde1532.
NVIDIA driver execution cannot be substituted by CI WARP tests. No R433 RTX
result has been reviewed. The request to run this standalone fixture is ON HOLD:
existing queue ownership and submission tests support continued implementation
without another user run. See `rt-project-history-coverage.md` for the distinction
between already tested contracts and the remaining integration gaps. Keep this
fixture available for the eventual queued-NVAPI integration validation; do not
treat its CI compilation as hardware execution or request it as a source-work gate.

Remaining game gates are unchanged: producer retirement for replaced work,
all native AS consumers, geometry/material mutation and lifetime ownership,
and measured gameplay quality/performance. This build makes none of those claims.
