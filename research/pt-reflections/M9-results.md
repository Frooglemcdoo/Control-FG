# M9 result and next integration gate

The supplied 2026-10-12 capture completed: 77,988 build calls, 56,425 geometry
rows, 37,447 binding rows in eight tables, 4,283 creation rows, 4,322 release
rows, zero recorded metadata read errors. Raw event quotas dropped 616,021
rows; this is a sampled record, not a complete scene history.

**M9 did not validate ownership in Control.** It recorded zero lease attempts.
All observed creation rows had resource pointers and GPU addresses. All observed
release rows had zero GPU addresses (39 had non-null resource pointers).
The release-time eligibility check therefore skipped every release. This does
not establish that COM ownership failed; that path was never exercised.

M10 retains at verified creation boundaries, where the native owner is present.
The reusable registry uses unique owner tokens, invalidates released wrappers,
and rejects overlapping allocation ranges and stale geometry tokens. It connects
this ownership to preceding BLAS observations and sampled native binding records,
then runs private-record bounds admission. It never feeds candidates to a GPU.
Ambiguous candidates remain rejected: a unique address candidate still does not
prove the TLAS instance/geometry/record correspondence or shared vertex indexing.

Capture limits: 64 MiB per retained resource, 512 MiB total, 8,192 owners,
65,536 geometry candidates. The runtime stops acquiring references and releases
its retained references after 120 seconds. Budget failures are counted explicitly.
Registry counters are independent of raw event quotas. Holding a COM reference
preserves the resource object, not a snapshot of mutable GPU data.

Remaining before private rendering: verified TLAS/SBT correspondence, descriptor
and resource-state agreement, and ownership retirement tied to submission fences.
M10 is a combined CPU integration build, not full PT or a visible rendering build.
