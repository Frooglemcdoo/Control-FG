# R271 — GI hit capture at the shading dispatch

This is a diagnostic build, not a speckle correction. Normal launch restores full native spotlight GI. The accepted GI sampling sequence, Native Blend, DirectStorage and Map Guard settings are unchanged.

Close the game and copy the complete package beside Control_DX12.exe. Run `ControlTOH/Diagnostics/Start-GI-Hit-Capture.cmd`.

Keep the desk lamp intact, RT indirect lighting ON, RR ON, Native Blend, GI RAW BYPASS and FG OFF. Use the same original plant view, with Jesse's hair visible on the left. In F10 select CAPTURE RR, close F10 and hold still. When complete, select PACKAGE RR and send all capture-part ZIPs and logs. This uses the existing four-pair sequence. Capture copies/export may hitch; do not judge performance from this run.

## What it saves

The existing GI/RR image pairs plus `gi-hit-slices.bin` and `gi-hit-slices.json` for each captured frame. The new files contain material, normal/UV and position/UV physical array slices from every observed GI shading batch, before the native container cleanup. Two 128×128 regions are centered at one-quarter and one-half of render width, halfway down the image. Metadata identifies their exact coordinates, resource formats, physical slices, source identities, batch/ray counts, row pitch, byte offsets and command lists.

The engine's current `g_uDGIPass`, `g_uDGIPassCount` and `g_uRTDiffuseRayCount` providers gate a contiguous batch ledger. Missing/duplicate batches, changed counts/frame, unsupported inputs, more than eight batches or sixteen slices, and buffer overflow fail the capture. Readbacks are exported after the existing capture queue fence completes. Native states are restored after each copy. Diagnostic allocation is bounded to 128 MiB per frame slot (two slots); it is enabled only by this launcher.

## Limits

These files capture physical array slices, not a verified shader-view-to-ray mapping. They do not yet contain spotlight records, per-light radiance operands or a complete replay input set. Metadata explicitly marks these missing. This build lets us inspect hit/material/normal/position discontinuities at the noisy pixels; it cannot by itself prove why a particular light produces a bright sample.

If capture reports failure, send the logs; do not treat partial files as a successful trace. No lighting contribution is removed or weakened by this capture.
