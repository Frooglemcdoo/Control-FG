M12R69 - recovery from R67/R68 surface-replacement regression

R68's log confirms PT_PRIMARY_SURFACE_ROUTING_STOP at frame 4028 with
reason=surface_table_reservation. Its 50,176-descriptor reservation failed;
the session then deliberately disabled the replacement until restart.
This was introduced by R68. Increasing capacity again is not the repair.

R69 restores the R66 lighting contract: native direct/specular lighting plus
PT diffuse indirect transport. PT remains enabled if it was enabled before.
The experimental full-surface replacement is not active in this package.
There are no surface draw hooks, full-resolution surface outputs, per-draw
surface descriptor copies, or 50,176-descriptor reservations in this route.
The R68 early frame reservation is also restricted to development builds.
The surface development host and shader remain in source and compile in CI.
A compile-time guard forbids enabling surface replacement in a recovery build.

The replacement currently exposes one-sample, frame-varying radiance with no
PT-owned temporal/spatial reconstruction. It also uses whole-path validity
as pixel coverage, so an unsupported secondary hit can change the selected
lighting model. Both must be addressed before replacement is re-enabled.
The log/video do not establish the relative contribution of each to flicker.

Close Control, then replace the package files beside dxgi.dll. Do not rerun
PT-Enable if PT is already enabled. Game settings, HDR, DLSS and RT settings
are not rewritten. An old pt_primary_surface.dxil may remain; this host never
loads it. Startup logs identify baseline=M12R66 and pt_indirect=1.
Collector: Collect-PT-Primary-M12R69.cmd.

This is a recovery build, not completion of all-lights PT, native-pass removal,
soft shadows or the FPS redesign. Windows compilation and WARP validation do
not establish in-game visual correctness or performance on your machine.
