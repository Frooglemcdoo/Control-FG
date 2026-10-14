M12R47 - explicit restart-only RT-only default

Install over R46 with Control closed. Existing RR/FG settings are retained.
Run RT-Only.cmd, then start Control. The overlay says RT ONLY (RESTART).
Compare DLAA and Quality in the same scene with FG off. Run the included
Collect-PT-Primary-M12R47.cmd afterward and send the result plus FPS readings.

No PT scene owner, descriptor mirroring, geometry preservation, PT shader
lookup, or glass state-object augmentation is installed in RT-only mode.
The startup log records capture=0. This uses the previously measured startup
bypass, now the default product mode rather than a diagnostic-only setting.
No FPS improvement beyond the earlier bypass is claimed before testing.

To use PT: close Control, run PT-Enable.cmd, and restart. The existing live
PT toggle works within that session; toggling it off still retains capture.
For a fully inactive PT subsystem, run RT-Only.cmd and restart again.
No live teardown or incomplete scene rehydration is introduced.

ControlFG-PT-Enabled.txt opts in to PT at startup. The older
ControlFG-PT-Capture-Off.txt override takes precedence. Both are read once.
Deleting the opt-in file also restores the RT-only default on next launch.
PT-Capture-Baseline/Restore remain aliases for the mode selection workflow.
