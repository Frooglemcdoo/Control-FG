Control-FG Primary PT M12R38 - command tracking concurrency

R37 startup bypass restored sampled DLAA/Quality scaling, isolating the
regression to the PT interception/capture path. This build removes global
command-state serialization from repeated setters. Each canonical command
list retains its own lock; COM aliases still share state. Original hook
functions and command-list identity lookups are cached per thread.
Geometry updates, ownership, descriptors, queue ordering and retirement
remain captured even while PT rendering is off. No stale-scene shortcut.

Install all files together beside Control_DX12.exe with the game closed.
IMPORTANT: run PT-Capture-Restore.cmd before launching this test, to remove
the R37 baseline marker. Otherwise PT capture stays disabled.

In the same stationary scene with FG off:
1. PT off: compare DLAA and Quality for at least 15 seconds each.
2. PT on: compare DLAA and Quality for at least 15 seconds each.
3. Toggle PT off again and check recovery.
4. Run Collect-PT-Primary-M12R38.cmd and upload its results ZIP.

PT_PRIMARY_HOT_CPU samples every 64 operations per thread. Bindings measures
tracker wall time; descriptors includes lock wait, native calls and observers.
These are cumulative sampled durations, NOT complete CPU or GPU frame times.
Existing queue/GPU/Reflex timing stays available. Startup baseline scripts
remain available for comparison; baseline mode cannot render PT.

Windows/WARP correctness checks do not establish in-game FPS improvement.
PT lighting architecture is unchanged by this performance build.
