M12R13 - avoid duplicate descriptor tracking; admit only reachable SBT rows

R12 keeps the descriptor index healthy but rejects writer_index_owner and
submits no PT dispatches. FPS fell in both native RT and PT-enabled modes.

Descriptor hooks now exclude private mirror writes from scene tracking and
suppress recursive observations while forwarding native descriptor calls.
Single-descriptor metadata copies avoid temporary vectors and allocations.
Native descriptor writes/copies still update the tracker while PT is off.
FPS recovery must be measured in-game; no performance claim is made yet.

Native writer rows now freeze missing owner tokens as zero. Preflight verifies
the complete selected TLAS-to-BLAS mapping, admits only reachable rows, and
preserves unused row numbers with private identifiers and zero roots. Every
reachable row still requires its original nonzero allocation tokens. Later
allocation registration cannot repair a missing captured identity. Regression
tests cover unused rows, reachable missing rows and later registration.
R12 does not prove its failing rows were unused, so PT remains unconfirmed.
New preflight diagnostics report remaining scene/ownership rejection details.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files and other settings. Load the entrance save
with native RT enabled. Note FPS with PT Reflections OFF, then use F10 ->
PT Reflections ON for 10 seconds and note FPS. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R13.cmd, and upload the results ZIP.
No screenshot is needed. Stop early if performance becomes unusable.
