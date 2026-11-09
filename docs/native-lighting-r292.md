# R292 — completed metadata read-plan inspection

Extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd, keep RT indirect lighting on,
move around for one minute, exit normally, and send the automatic diagnostic ZIP.
No hotkeys or stationary camera required. Normal launch leaves the observer off.

R291 recorded 676495 successful build-time range lookups and zero misses,
ambiguities or invalid ranges across 2557 AS submissions in the supplied run.
The final metadata fence completed. This is successful coverage for that run,
not proof of complete game coverage, stable geometry bytes or active scene membership.

R292 builds a CPU read plan from a fence-completed immutable ledger snapshot.
It preserves AS address/revision, geometry index and the vertex/index/transform/
instance ranges, including copied AS lineage. Plans are capped at 65536 ranges;
invalid descriptors, overflow, incomplete ledger or allocation failure reject
and clear the plan. Every planned range is checked again against retained owners.
Reports distinguish default-heap, upload-heap and other/unknown ranges.

GI_PLAN_R292 reports snapshot availability, plan success, retained BLAS/TLAS counts,
range coverage, distinct owners, and requested bytes including overlapping ranges.
The ledger can retain historical structures; these are not active-scene counts.
Heap type is not resource state, lifetime or byte-generation proof. No Map, GPU
copy, barrier, queue wait or rendering consumer is added. copy_ready and reuse_ready
remain zero. A snapshot not ready at a report is retried at the next report.

GI_METADATA_R292 and GI_COVERAGE_R292 split the previous oversized status line,
so the trailing completion/miss fields are no longer truncated in ordinary runs.
The R291 index-priority policy, 8192 owner slots, 512 MiB logical extent budget,
16384-address metadata history, and 65536-event journals remain in place.
Replacement GI remains disabled; this is preparation for GPU byte capture and
triangle reconstruction, not a speckle-fix build.

Validation: focused Windows run 41495677744 passed, including scene read-plan
lineage, all four range kinds, bounds/capacity rejection, completed runtime
snapshot inspection and both GPU adapters. Full Windows run 41495677206 passed
all 53 steps. Actual logs were checked for compiler/assertion/HRESULT failures.
Build commit: 7eb061b0b2fce57a018c46744756d7b3ae2bf323.
