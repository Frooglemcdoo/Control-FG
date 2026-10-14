Control primary PT - M12R45

R44 CPU trace identifies globally shared counter updates inside the parallel
descriptor-copy path as a hot spot. R45 removes its three shared increments:
slot generations now advance per descriptor; diagnostic copy totals use
cache-line-separated thread shards. Snapshot locking and native copy forwarding
are unchanged. Geometry capture and all shaders are unchanged.

Close Control and replace the package files beside the installed dxgi.dll.
Run PT-Capture-Restore.cmd and restart so capture is enabled.
Keep PT OFF, FG OFF and RR unchanged. Compare DLAA and Quality in the same spot.
Send both FPS readings and Collect-PT-Primary-M12R45.cmd output.
No new ETL capture is requested for this test.

Windows/WARP validation is recorded in Build.json. In-game FPS is unverified.
