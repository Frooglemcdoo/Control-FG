CONTROL RT SCENE QUERY IDENTIFICATION - KEEP R392 INSTALLED

1. Extract this ZIP to a writable folder outside the game directory.
2. Start Control DX12 with R392 and load into gameplay.
3. Run Run-RT-Scene-Probe.cmd, then return to the game.
4. Walk around and destroy a few objects during the 30-second capture.
5. Send RT-Scene-Probe.txt from the extracted folder, even if it reports FAIL.

No installation or administrator access normally needed. Run the game and probe
at the same permission level. The probe reads bounded metadata from Control;
it injects nothing, builds no acceleration structures and changes no settings.
It requires the exact renderer image already analyzed. An unsupported image is
rejected before scene pointers are read. Close extra Control DX12 instances.

Purpose: identify the concrete native scene-query function behind the virtual
call used by RT preparation. Existing logs contain the callsite and aggregate
CPU timings but do not identify its runtime target. That target is needed to
trace scene membership and change tracking before retiring native preparation.

This is NOT an FPS benchmark, an atomic scene snapshot, a replacement mod or
proof that native preparation can be skipped. Sample counts can race native
updates and are observational only. No full game-memory dump is produced.
