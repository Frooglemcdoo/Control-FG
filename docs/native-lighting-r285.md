# R285: screen-wide GI and RR stack capture

Run `ControlTOH/Diagnostics/Start-GI-Joint-Trace.cmd` with the game closed. Keep RR and RT indirect lighting enabled, Native Blend selected, frame generation off, GI RAW BYPASS, and REF CLAMP BYPASS. Frame the plant and Jesse's hair together anywhere on screen. Press F7 once, close F10, and hold the camera and mouse zoom steady until SAVED. Exit normally; send every ZIP produced by the launcher.

This is a diagnostic build. Native lighting instructions are preserved exactly.

## Coverage

* Eight GI source frames sample the whole dispatch, including hair and foliage anywhere on screen. A 16-pixel grid is used at 2560×1440; larger dispatches automatically use a coarser grid to bound storage. Grid offsets change across frames. This is sparse spatial sampling, not every pixel's complete ray history.
* Every sampled pixel includes all GI rays, hit positions/normals/material IDs, throughput, volume lookup/blending, spotlight projection/cookie/shadow stages, point-light distance/range/attenuation/angular/shadow-moment stages, per-light products, surface sums, and final GI accounting.
* One consecutive RR input/output pair captures full-resolution native G-buffers, depth/motion, actual RR material guides, raw albedo when bound, hair responsivity when bound, diffuse/specular lighting, pre-filter GI, GI at RR input, scene input, RR output, projection/jitter/exposure and filter-state metadata.
* The first GI source trace must match the first RR frame's engine frame ID. The other seven source frames are spaced samples, not consecutive RR pairs. Collection verifies this join and explicitly fails on missing stages or overflow.

The red outline now covers the whole screen. A best-effort desktop BMP supplies framing context only; it is not synchronized GPU/HDR evidence.

## Bounded operation

Source recording uses one fixed 256 MiB GPU buffer and one readback buffer, reused only after fence completion and export. RR uses the existing two prepared slots once, with no four-pair auto-repeat. Readback/export remains asynchronous and no render-thread fence wait is introduced. Diagnostic recording and allocation can still cause temporary hitches. A completed source sequence is not labelled SAVED until the RR pair completes too.

`CONTROLTOH_GI_SAMPLE_STEP` in the launcher accepts 8, 16, 32, or 64, so sampling density can change without rebuilding the DLL. Smaller values produce more data and may overflow in complex lighting. The default automatically bounds the sampled pixel count. Full-resolution RR images are unaffected by this setting.

The scope is the existing GI/RR rendering path. This does not capture every engine system, every material texture, or every internal operation inside NVIDIA's RR implementation. Raw native ray-array textures are not duplicated; the source trace records their consumed hit data at sampled pixels.
