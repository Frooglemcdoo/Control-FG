# R332: compact performance capture

R331 showed no user-observed performance improvement. This build fixes measurement size and usability; it does not claim a new GI performance fix.

F10 RECORD GI keeps the same five-second countdown, stand-still beep, slow-pan beep, and double-beep completion. Close the game normally; packaging creates ONE compact ZIP plus a readable SUMMARY.txt on the desktop. No WPR, ETL, GPU resource capture, or administrator prompt. Full logs and traces are never included.

The native Present hook records intervals only after the button is clicked, batches writes at most every 64 frames or one second, and stops after two minutes. Reports include per-phase mean native frame time/FPS, p95/p99, long-frame counts, coverage, selected GI mode/settings, build hash, process samples, and up to 4 MiB of focused log text. The ZIP is normally much smaller. Frame Generation should be OFF. These are game-frame cadence measurements, not displayed/generated FPS or individual GPU pass durations. Nested CPU timings must not be added.

Use the same room/settings to compare modes/builds. Incomplete frame coverage and nonforeground samples are flagged. Missing samples produce no FPS claim. A completely clean Native run remains necessary to separate tracking overhead from Owned rendering cost.

Install all contents beside Control_DX12.exe preserving folders. Use the existing Owned launcher. Lighting shaders, ray counts, and scene admission behavior are unchanged from R331.
