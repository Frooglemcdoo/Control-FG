M12R61 - strict scene validation without duplicate reconstruction

R60 full-frame captures confirm the reversed labels: the DLAA-named capture
is Quality (2560x1440); the Quality-named capture is DLAA (3840x2160).
The main thread runs for about 94% of both ten-second recording windows.
The PT GPU pass scales from about 3.9 ms to 1.8 ms, but CPU scene preparation
and submission validation remain significant during GPU queue gaps.

This build re-selects every TLAS/BLAS version at strict submission time and
checks instance bytes, allocation identities and immutable geometry identity
against the retained prepared scene. On a match it avoids constructing and
then destroying a second scene's maps and vectors. A mismatch uses the original
full scene reconstruction/comparison. Unresolved fences remain pending;
resource-owner changes still trigger full plan revalidation.

Scene resolution also reuses a bounded recursion-path vector instead of
allocating a tree node for every resolved BLAS. Cycle/depth checks remain.
Regression coverage includes late fences, changed BLAS under the same TLAS,
masked instances, modified proofs, compaction and retained geometry ownership.

Close Control, extract beside dxgi.dll, then launch normally. Keep PT enabled.
Compare DLAA and Quality in the same scene for about 30 seconds each, with FG
and caps/VSync off. Close and run Collect-PT-Primary-M12R61.cmd. Send the ZIP
and average FPS. Use normal play, not WPR, for this FPS comparison.
In-game gain remains unverified until this test. No PT-enable script needed.
