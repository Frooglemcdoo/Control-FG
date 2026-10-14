Control PT M12R43 - concurrent fixed-table descriptor copies

R42's PT-off results confirm its bypass was ineffective: one sampled untracked
operation versus 107378 sampled tracked operations. Almost all native copies
write the fixed-table heap suffix that R42 conservatively kept synchronized.
DLSS did change from 3840x2160 to 2560x1440; the CPU tracking remained active.

R43 lets independent one-slot mirrored copies run concurrently when neither
endpoint carries AS, typed-UAV or cached-source metadata. Each native copy and
its CPU mirror finish within one shared transaction. Snapshots, metadata edits,
heap creation and bulk copies retain exclusive access. Adjacent seen flags use
separate bytes, version/copy counters are atomic, and the legacy glass observer
runs after the shared transaction ends. PT callbacks with no pending work no
longer acquire the descriptor gate.

This does not disable scene capture or defer PT initialization. GPU shaders,
PT lighting and resource ownership are unchanged. Existing live PT behavior
is retained. Performance recovery still requires in-game validation.

INSTALL AND TEST
Close Control. Extract the entire package beside Control_DX12.exe, replacing
the previous experimental files. Keep existing Streamline/RR files. Run
PT-Capture-Restore.cmd if the startup bypass marker is still present.

Test PT OFF first, with FG OFF, no frame cap/VSync and no profiler recording.
Use the same stationary scene: settle in DLAA, then switch to Quality and
settle again. Run Collect-PT-Primary-M12R43.cmd and send its small ZIP with FPS.
There is no need for another PT-on test or large trace until off-mode scaling
is fixed. The collector includes descriptor_parallel_wait/native/mirror sampled
counters to confirm that the new concurrent path actually ran.

CPU timing totals are sampled inclusive wall time across threads, not frame
CPU time. The unchanged PT-Profile tool is available for follow-up only.
