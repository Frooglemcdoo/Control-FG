Control PT M9 - RETAINED RESOURCE OWNERSHIP TEST

1. Close Control and back up dxgi.dll.
2. Replace only dxgi.dll beside Control_DX12.exe; keep existing shader files.
3. Load entrance with RT ON and PT OFF. Move briefly, then leave it running
   THREE MINUTES before closing the game.
4. Run Collect-PT-Scene-M9.cmd after closing. Send its results ZIP.

M8 established allocation range coverage but not ownership. M9 takes an owned
ID3D12Resource interface reference just before selected native buffer releases,
checks its address/size after the native helper returns, then drops that
reference immediately. It performs no GPU work, readback, waits or state changes.
The test permits at most 32 attempts, 8 MiB per buffer and 64 MiB concurrently
retained. This briefly extends selected resource lifetimes. It does not prove
queue/fence retirement, live scene correspondence, or full PT readiness.

The capture now ends 120 seconds after installation; table samples are at least
10 seconds apart. Independent geometry, binding and resource quotas remain.
Incomplete captures show unknown totals instead of misleading zeros.
Requires the verified Steam game DLL and the existing mod installation.
Rollback: close Control and restore your backed-up dxgi.dll.
