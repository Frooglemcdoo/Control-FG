R60 full-frame capture add-on (no DLL replacement)

R60 did not improve overall FPS. Its existing log measures only the private PT
GPU dispatch and selected CPU hooks, not the entire frame critical path. This
capture records Windows GeneralProfile + GPU together, including CPU samples,
thread scheduling, GPU queue events and present events. Exact capture QPC/UTC
boundaries and the current probe log are bundled for correlation.

1. Extract these files beside the installed R60 dxgi.dll and Build.json.
2. Start Control with PT ON. Keep FG OFF and VSync/frame caps OFF, RR unchanged.
   Load the same scene, set DLAA, and let it settle.
3. Right-click PT-Frame-DLAA.cmd and Run as administrator. Press Enter when
   ready, then return to the game during the 10-second countdown. Stay in the
   game with the camera still for the following 10-second recording.
   Saving may cause a pause AFTER the recorded interval. Wait for Saved/Upload.
4. Change only DLSS to Quality, let it settle, then repeat using
   PT-Frame-Quality.cmd. Keep PT ON for both captures.
5. Upload every ZIP from each displayed Upload folder under Desktop\Control-PT-Frame-Profiles.
   Large results are automatically split into approximately 48 MiB ZIP parts.
   Send both sets even if FPS is unchanged. Original ETL/full ZIP stay local.

The script checks the loaded R60 DLL hash and current process log. It refuses
an existing Windows recording and never cancels someone else's trace. It does
not change game settings, PT mode, DLSS, HDR or rendering. DLAA/Quality are labels
chosen by the launcher; actual render dimensions must be checked in the log.
No enable script and no new renderer build are needed.
