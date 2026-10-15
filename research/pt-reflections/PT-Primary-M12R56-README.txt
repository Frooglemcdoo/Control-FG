M12R56 - reuse completed PT bookkeeping allocations

R55 confirmed upload pooling works: shader-table preparation about 0.10 ms,
camera about 0.003 ms, total preparation about 5.5 ms (previously about 7.2 ms).
User observed 45 FPS DLAA and 46-50 Quality. Remaining metrics initialization
cost was about 1.5-1.6 ms; scene preparation remained about 3.6-3.7 ms.

This build reuses metrics resources, five prebuilt admission command lists and
allocators, timestamp query heap and descriptor heap. It keeps at most 16 idle
entries, matched by device identity. Return requires explicit completed-consumer
proof and release of all existing recording owners. Further admission or Begin
invalidates that proof. Unknown completion stays under existing quarantine.

The GPU clears its admission word after tracing, then copies that cleared word
to readback. Only a verified zero may return to the pool. This ensures a recycled
buffer cannot inherit prior full-frame/probe admission. Every Begin still clears
coverage/fault counters. Fresh admission is required for every new lifetime.
R55 upload pooling and R54 timing windows remain; metrics pool counters are added.
Shader binaries, ray count, scene validation and native settings are unchanged.

Close Control and extract beside dxgi.dll. Test with PT on in the same scene:
DLAA 30 seconds, Quality 30 seconds, then DLAA 30 seconds. Keep other settings
unchanged and note FPS. Close and run Collect-PT-Primary-M12R56.cmd; send its ZIP.
No enable script needed. PT mode remains persistent and restart-only.
FPS improvement and in-game output still require confirmation.
