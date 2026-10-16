M12R94 - texture fallback diagnostic build

This build does NOT claim to fix texture switching, missing floor reflections,
or baked lighting. It preserves R93 rendering and adds bounded diagnostics:
- missing binding origin (never set, root change, heap change, explicit zero),
  previous recorded address and reviewed shader identity;
- rejected primary material ID, both flag words, rejection reason and screen
  position in up to 32 tiles per logged diagnostic frame.

Producer PT coverage is not proof that the final raster draw used PT. The RR
material-guide route currently lacks final per-pixel composition ownership.
A correct ownership solution must account for depth tests and later native
writes, not just set another producer coverage bit. No stale sampler address
is reused and unsupported material gates remain intact.

Close Control, replace package files, keep settings, do not rerun PT-Enable.
Walk through the affected floor sections and collect with the R94 script.
If possible keep a short video of that same run, so screen coordinates can be
associated with the reported sections. A clean build proves only compilation
and regression checks, not visual correction.
