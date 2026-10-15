M12R64 - release descriptor gate during PT preparation

R63 logs confirm actual PT dispatch resolution changes with DLSS. The primary
path nevertheless holds the exclusive descriptor gate across scene preparation,
SBT/camera uploads and metrics allocation, blocking native descriptor workers.
R64 releases that gate after taking immutable CPU descriptor snapshots, then
reacquires it and checks both primary and environment slot versions before
recording any GPU references. Changed required inputs reject the attempt.
Thread-local hook muting is explicitly lifted before gate reacquisition.

Scene/recording locks, late submission checks, GPU waits, shaders, ray counts,
startup settings and render quality are unchanged. Preparation logs include
descriptor_unlocked_ms and descriptor_relock_ms. These are CPU wall times,
not an FPS gain claim. Windows concurrency tests verify worker progress and
required-slot invalidation through the same release/reacquire sequence.

Close Control, extract beside dxgi.dll and launch normally with PT enabled.
Compare DLAA, Quality and Performance in the same scene for 30 seconds each,
FG and caps/VSync off. Close and run Collect-PT-Primary-M12R64.cmd. Send the ZIP
and average FPS for each mode. No PT-enable script needed.
