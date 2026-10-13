Control-FG Primary PT M12R39 - bulk descriptor mirroring

R38 did not restore resolution scaling. Its sampled descriptor scopes were
much more expensive than command-state tracking. This build removes the
per-source-fragment mirror path in bulk descriptor copies. It clips to PT
fixed tables first, copies sparse typed-UAV format metadata once per native
transaction, and submits one shadow CopyDescriptors batch per affected heap.
The native game copy, descriptor ownership/generations, table invalidation,
and current geometry capture remain intact. Non-resource descriptor copies
no longer acquire the PT descriptor gate.

Close Control and install all files beside Control_DX12.exe.
Run PT-Capture-Restore.cmd before launching if the baseline marker remains.
In one stationary scene with FG off, test DLAA and Quality for 15 seconds
with PT off, then repeat with PT on. Toggle PT off and check recovery.
Run Collect-PT-Primary-M12R39.cmd and upload the results ZIP.

PT_PRIMARY_MIRROR_COPY records actual private copy call/descriptor counts.
PT_PRIMARY_HOT_CPU retains sampled wall time (1/64 per thread); descriptor
scope includes lock wait, native calls and observer work, not GPU duration.
Windows/WARP correctness tests do not establish in-game FPS improvement.
PT lighting is unchanged by this performance build.
