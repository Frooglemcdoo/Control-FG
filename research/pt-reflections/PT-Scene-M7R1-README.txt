Control PT M7R1 - NATIVE MATERIAL / GEOMETRY CORRELATION DIAGNOSTIC

Requires the existing mod installation and the verified Steam game DLLs.
This is the next in-game diagnostic, not full PT or a visual upgrade.
It captures native shader-table input bindings and BLAS geometry in one run.

1. Close Control. Back up your current dxgi.dll outside the game directory.
2. Copy this dxgi.dll beside Control_DX12.exe, replacing the mod's dxgi.dll.
   Keep your existing shader sidecars and other game files.
3. Launch Control and load the entrance scene promptly. Enable ray tracing;
   leave PT OFF. Move around for 30 seconds, then leave the scene running
   for THREE MINUTES before closing the game.
4. After closing Control, run Collect-PT-Scene-M7R1.cmd from this package folder.
5. Send the PT-Scene-M7R1-Results-*.zip created beside the collector.

The collector can read a live log, but closing the game after the capture
window ensures the archive contains the final records. No PIX is needed.

The observer reads CPU metadata and forwards the original calls. It adds no
GPU work or readback. Diagnostic CPU, memory and disk overhead is expected.
It samples up to eight native binding tables, at least 15 seconds apart,
with at most 8,191 entries each. Geometry and binding records have separate
65,536-row limits. The recording window ends 180 seconds after installation.
If loading is slow or a limit is reached, send the results as-is.

This tests whether index/material binding addresses have unique, ambiguous,
or missing position-stream candidates. It does not prove GPU completion,
resource lifetime, correct shading, or readiness to render full PT.
Unsupported Steam module hashes fail the observer installation and are logged.

Rollback: close Control and restore your backed-up dxgi.dll.
Logs: %LOCALAPPDATA%\ControlFGProbe\pt-scene-m7r1-*.jsonl

R1 observes both inline and worker-thread native table-writer calls.
The summary includes calls per route and rejected nonpositive counts.
