# R326 — reduce repeated scene mapping work

Performance is the focus. R325 PID 76084 has 585 successful admissions and no logged ownership failure. Owned mode resumed after the native toggle. Inclusive CPU averages: preparation 4.295 ms, admission 16.599 ms, mapping 9.795 ms, selection 2.807 ms, table commit 3.447 ms. These scopes are not additive frame times or GPU timings. Different captures are not a controlled benchmark. The earlier permanent WAIT failure remains possible; it did not recur in R325.

Two mapping changes:

- Resolve exact geometry without collecting the unrelated-candidate census on successful admissions. Failed resolutions rerun with full diagnostics under the same owner lock, retaining failure details and per-geometry failure counts. Normal validation still rejects missing, overlapping, stale or out-of-bounds owners.
- Reuse validated mapping/conversion for nonadjacent instances inside one locked admission transaction. Keys include BLAS address, allocation token, build revision, geometry index and all 96 source-row bytes. Witness and metadata checks, row-conflict checks, allocation-token collection and downstream storage admission remain. The cache is capped at 131072 entries and discarded before the next transaction. No scene cache survives allocation changes or submission cuts.

This does not change ray count, resolution, shading, native fallback, GPU queue waits or scene ownership lifetime. R325 retired scratch reuse remains. Native GI work and other admission costs still exist. This is a targeted CPU optimization, not a claim that the FPS regression is solved.

Validation: scene-packet tests cover separated A/B/A meshes, different material rows, both index widths, owner removal between transactions, retained failure census, conflicts, frozen geometry and stale identity rejection. Windows build and existing foundation/producer tests are required before delivery.

Install preserving folders. Run ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd with RR, RT indirect lighting and GI RAW BYPASS. Keep ray count fixed. Compare FPS in the same area, switch Native then Owned, and exit normally. The automatic ZIP contains timings and failure diagnostics; no Capture RR or extra standalone run is needed.
