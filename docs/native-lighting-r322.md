# R322 — owned GI CPU and dispatch overhead

R321 PID 65984 proves the index-bound fix admitted the owned path. The first three censuses passed every geometry (7572, 7572, 11344). User reports the issue fixed but 18 FPS. Across the recorded CPU metric windows, 1033 scene-admission calls averaged 20.398 ms and 1033 preparation calls averaged 9.296 ms. These are inclusive CPU scopes, not GPU timings and not an additive decomposition of frame time. Geometry preservation also remains active. The run records 2560x1440 with one ray initially and six rays later.

Changes:
- Catalog selection and producer-table preparation reuse exact BLAS selections only within the current immutable cut. Cache keys retain allocation identities and, in table preparation, revisions. No cache survives a call/frame; current-cut lifetime/alias/ordering checks remain mandatory.
- Deferred SBT commit runs the same backing-range, storage-lease and geometry validation, then writes the upload already reserved for the pass. It no longer creates, maps, fills and destroys a second temporary upload merely to validate the table.
- The tile pixel cap grows from 16384 to 65536, retaining the 262144-sample cap. At 1440p, one-ray processing uses 57 tiles and six-ray processing 85, instead of 225. Resolution, total samples, shading, output coverage and ray count remain unchanged. Larger scratch allocations consume more temporary GPU memory; the existing 512 MiB scratch budget remains enforced.
- CPU admission timing now separately reports scene_selection, scene_mapping, table_commit and control_commit beneath the existing inclusive scene_admission scope. The R320 geometry census remains for any newly exposed rejection.

Validation: existing scene closure, stale/overlapping ownership, halfword source-tail, transactional rejection and tiled coverage tests; added 128 shared-instance selections across old/new cuts and explicit 1440p tile-count assertions. Windows CI covers the existing GI and queue tests plus the complete mod build. Live speed and visual quality still require the next run.

This is an initial performance correction, not a promise of restored 80–90 FPS. Native GI still executes for fallback; owned per-ray shading and geometry preservation remain. No spatial/temporal sample reuse is active yet.

Install preserving folders. Launch ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd, with RR and RT indirect lighting on and GI RAW BYPASS. Keep the same sampling settings for comparison. Report FPS and whether OWNED remains active; exit normally and send the automatic ZIP. No Capture RR or repeated standalone producer test is needed.
