Control PT M6 - IN-GAME GEOMETRY BINDING DIAGNOSTIC

This is a Control test, not the standalone PT-DXR test and not full PT.
Requires the existing mod installation and the verified Steam game DLLs.
Built from M5 / the existing Opaque R2 integration branch. Existing PT/RR/FG
rendering code and sidecars are unchanged; the added observer reads CPU metadata.

1. Close Control. Back up your current dxgi.dll outside the game directory.
2. Copy this dxgi.dll beside Control_DX12.exe, replacing the mod's dxgi.dll.
   Leave every shader sidecar and other game file in place.
3. Launch Control normally. Load the entrance scene promptly, with ray tracing
   enabled. Leave PT OFF for this diagnostic. Move around for about 30 seconds,
   then keep the scene open for THREE MINUTES before closing the game.
4. Double-click Collect-PT-Scene-M6.cmd in this extracted package folder.
5. Send the PT-Scene-M6-Results-*.zip it creates beside the collector.

No PIX, hotkey, or new capture is needed. Do not rename dxgi.dll for this test.
The observer records the native index/position bindings at both verified build
call sites. A background writer drains bounded CPU memory to a JSONL file; it
adds no GPU work, GPU readback or GPU waits. It stops after 180 seconds or 65,536
records. Modest diagnostic CPU/memory/disk overhead is expected.
The summary explicitly identifies limits, missing final records and read errors.

Success for this diagnostic is nonzero fresh triangle records with usable
index/position formats. This does not prove buffer lifetime, submitted GPU work,
shader-table correspondence, correct material shading or full PT integration.
Unsupported game hashes fail the observer installation and are logged.
If loading takes too long or no fresh triangles are recorded, send the results
as-is so the next step can be based on the actual evidence.

Rollback: close Control and restore the dxgi.dll you backed up.
Logs: %LOCALAPPDATA%\ControlFGProbe\pt-scene-m6-*.jsonl
