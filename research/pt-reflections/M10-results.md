# M10 results and M11 scope

M10 completed with no recorded metadata read errors. It retained 3,283 resources,
rejected no interfaces, and validated three retained references across native
releases with zero mismatches. This establishes the exercised CPU ownership
path; it does not establish submission/fence retirement or a GPU data snapshot.

The 512 MiB cap was reached (536,870,908 bytes). There were 1,013 budget rejects.
Of 38,664 sampled bindings, 23,092 had an owned geometry candidate, 14,421 lacked
allocation coverage, 16 lacked geometry and 1,135 failed bounds admission.
Zero candidates were marked ambiguous. Raw quotas dropped 766,719 rows.

M11 raises the retention cap to 1 GiB, bounded to the same 120-second capture.
Observed creation sizes in this capture total about 868 MiB. Coverage is still
measured, never assumed. R16 padding now uses the retained parent's physical
range: a final aligned word can extend past the declared index-view length only
when the parent owns those bytes. The private bound includes that proved padding.

The verified renderer fills instances through lock at RVA A8972 and memcpy at
A8987 (target 588E0E). M11 forwards both calls unchanged and copies the CPU source,
not write-combined GPU upload memory. The native lock provides the destination
buffer identity and extent. Source snapshots are matched to InstanceDescs at
native TLAS builds. Native AS compaction at d3d RVA 3369D carries preceding BLAS
geometry metadata to the compacted destination. Both native DLL hashes are gated.

The CPU relation checker applies instance contribution / ray count + geometry
ordinal to the binding table, then checks index address, stride and range. It
counts missing BLAS, out-of-range records, mismatches, and conflicting position
bindings. This is still a comparison of preceding CPU snapshots across native
threads, not proof of GPU ordering, same-dispatch identity, or vertex indexing.
No shader tables are substituted and no GPU work is added.

Remaining rendering gates: same-dispatch scene identity, complete ownership,
position/attribute indexing, resource states and fence-bound submission lifetime.
M11 is not a visible PT build. Do not interpret CPU matches as rendering ready.
