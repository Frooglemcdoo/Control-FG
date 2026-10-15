M12R66 - eliminate redundant immutable proof self-comparison

R65 result: Performance / Quality / DLAA PT GPU averages 1.107 / 1.903 /
4.174 ms; CPU preparation averages 2.681 / 2.723 / 2.712 ms. Lower-resolution
ray dispatch is correct. No material in-game FPS improvement was reported.

The late submission reuse path calls SameDispatch(prepared,prepared), and
on a strict MatchScene success calls SameScene with the retained scene as
both arguments. Those equality routines unnecessarily walk records and maps
that are identical by object identity. R66 returns early for that case only,
AFTER the existing validity checks. Zero AS versions, empty SBTs and missing
selection identities still reject. Different objects still compare fully.
Strict current-cut TLAS/BLAS selection, owner revision validation and fallback
rebuilds remain mandatory. This does not reuse an old scene across frames.

Local synthetic benchmark: 4096 BLAS entries, 12000 instances, 24000 hit
records, 3000 repetitions: paired self-comparisons fell from 0.267 to 0.023 ms.
This is a Linux CPU microbenchmark, not a Windows frame-time or FPS prediction.
Regression tests cover invalid self-comparisons and changed independent inputs.
The optimization targets late validation, not the roughly 2.7-ms preparation
reported above. The remaining preparation cost is still under investigation.

Close Control, extract beside dxgi.dll and launch with PT enabled. In the same
scene test Quality, Performance, then Quality again for 30 seconds each, with
FG and caps/VSync off. Close and run Collect-PT-Primary-M12R66.cmd. Send the ZIP
and average FPS for each segment. Existing settings are preserved.
