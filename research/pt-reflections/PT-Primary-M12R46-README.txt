Control primary PT - M12R46

R45 did not improve reported FPS. R46 removes normal-play shader dump scanning.
The R44 CPU trace showed 705 of 32279 samples in diagnostic IsHitLighting.
The dump helper scanned bytecode before checking its seen-ID list, including
repeated negative candidates. Dumps are only log output; no rendering consumer
reads this state. Normal play now exits before reading shader bytecode.

Close Control and replace package files. Normal capture must be enabled:
if baseline mode is still selected, run PT-Capture-Restore.cmd and restart.
Compare PT OFF, FG OFF, DLAA vs Quality in the same spot; keep RR unchanged.
Send FPS and Collect-PT-Primary-M12R46.cmd results. No ETL trace needed.

Diagnostic shader dumping remains available only when the launching process
inherits CONTROLFG_PT_SHADER_DUMP=1. Leave it unset for performance testing.
All renderer shaders and scene ownership are unchanged. FPS is unverified.
