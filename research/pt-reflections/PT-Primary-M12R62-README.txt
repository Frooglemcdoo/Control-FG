M12R62 - remove geometry and prepared hit-table copies

R61 logs show modest CPU preparation reductions with no new admission failures.
R62 validates topology through borrowed pointers into the retained immutable
scene geometry. This avoids copying frozen-geometry ownership for each repeated
instance. Every instance/row and conflict check still runs.

The completed dispatch plan is moved into its immutable submission proof once.
SBT upload and TLAS binding read that same retained plan. This removes the second
full hit-table, allocation-token, BLAS-version-map and ownership-list copy.
Strict late scene/owner revalidation and fence retirement remain in place.

Close Control, extract beside dxgi.dll and launch normally with PT enabled.
Compare DLAA and Quality in the same scene for about 30 seconds each, with FG
and caps/VSync off. Close and run Collect-PT-Primary-M12R62.cmd. Send the ZIP
and average FPS. No PT-enable script needed. In-game gain is not yet verified.
