# R246 combat capture (no game DLL changes)

Keep the R245 build and use its 4ms pinned launcher with your usual graphics settings.

1. Extract the capture package to a writable folder. Start Control and load near the troublesome fight.
2. Right-click `Capture-Combat-R246.cmd`, choose **Run as administrator**, then return to the game.
3. Press **F8**. After the first beep, remain in the quiet scene for five seconds. At the second beep, enter combat and reproduce the slowdown. The recording ends automatically after another 25 seconds (third beep).
4. Return to the capture window, wait for `DONE`, and send the `Combat-R246-*.zip` created beside the scripts. If capture fails, send the generated folder's small text logs and capture.json.

Do not run video capture alongside this test. If a trace is too large to upload, retain it and report its size. The trace includes system-wide CPU stacks, scheduling and GPU activity, process/module names and paths; close unrelated applications beforehand.

This is an attribution trace, not an FPS benchmark: ETW itself adds overhead. Windows' built-in WPR records the CPU and GPU profiles in bounded memory mode. A named session avoids cancelling other recordings; a conflict stops this tool without cancelling an existing session. No registry, affinity, priority, graphics, streaming budget, or DLL changes are made. Do not forcibly close the console while recording or saving; if interrupted, the named recording may need manual cleanup.

The manifest records process ID, loaded-module bases/sizes, and UTC/QPC timestamps for the start, combat cue and end. These allow alignment with the existing probe log. Native game symbols may be unavailable; sampled module offsets can still be matched to the targeted decompile. Inspect CPU sampled stacks, CPU scheduling/ready/wait activity and GPU queue occupancy together before selecting an optimization. Verify lost events in WPR status and trace analysis; repeat only if missing data prevents attribution.

R245 evidence: sampled frame intervals reached about 24ms with RR evaluation around 3.1–3.3ms; this is not a full GPU frame measurement. Replay-worker CPU time sometimes rose. Across the entire run, 32 native texture allocations exceeded 16ms (maximum 90ms), but their relationship to combat is unknown. Cumulative maxima and sparse per-stage samples cannot identify the combat critical path. R246 supplies the missing timeline without altering the successful DirectStorage/visual baseline.

Reference: https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options
