Control PT M8 - BUFFER LIFECYCLE COVERAGE DIAGNOSTIC

1. Close Control and back up your current dxgi.dll.
2. Replace only dxgi.dll beside Control_DX12.exe. Keep the shader sidecars.
3. Load the entrance promptly with RT ON and PT OFF. Move around for 30 seconds,
   then keep the scene running for THREE MINUTES before closing Control.
4. Run Collect-PT-Scene-M8.cmd after closing the game. Send its results ZIP.

M7R1 captured material bindings successfully. M8 additionally observes 27
verified native calls: two resource-creation helpers and buffer releases.
It records wrapper/resource identities, GPU addresses and buffer sizes, to
measure creation/release coverage for index, attribute and position streams.
This is a partial native lifecycle observation, not complete D3D12 lifetime
tracking, fence completion, or proof that a resource can safely be reused.
It does not retain resources, change GPU commands, or enable full PT.

Requires the existing mod and verified Steam game DLL. Recording lasts 180
seconds from observer installation, with independent 65,536-row limits for
geometry, bindings and lifecycle events. Diagnostic CPU/memory/disk overhead
is expected. Send results even if the limits were reached.
Rollback: close Control and restore your backed-up dxgi.dll.
