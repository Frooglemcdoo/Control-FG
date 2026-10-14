Control PT M12R44 - correct concurrent-copy eligibility

R43 results still show dominant exclusive descriptor tracking: 216133 sampled
exclusive entries versus 9253 completed parallel entries. Its historical
interest filter unnecessarily rejected cached sources and previously typed
slots even when the copy required no metadata change.

R44 checks current metadata under shared ownership. Cached SOURCE descriptors,
erased UAV history and unchanged typed formats can run concurrently. The actual
native descriptor and its CPU mirror are still copied, with a fresh version.
AS identities, destination-token invalidation and changed typed formats retain
the exclusive path. Snapshot synchronization and live PT capture are retained.

INSTALL AND TEST
Close Control and extract all files beside Control_DX12.exe, replacing R43.
Keep your existing Streamline/RR files. Use normal capture, PT OFF, FG OFF,
no profiler recording and unchanged scene/settings. Compare DLAA and Quality.
Send the FPS readings and Collect-PT-Primary-M12R44.cmd results ZIP.
No PT-on test or large trace is needed for this check.

This is a targeted CPU change. FPS recovery is not yet verified in Control.
Shaders and lighting are unchanged. Counters are sampled wall time across
threads, not frame time; don't sum them or scale them into a frame-time claim.
