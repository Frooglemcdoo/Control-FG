# Control exploration helper — test 1

Standalone helper for the supplied Control DX12 executable. Works alongside R49;
no replacement dxgi.dll or graphics changes. Exact EXE SHA-256 and all four code
sites must match. Unsupported game builds are refused.

1. Back up your save. Start Control DX12 and stay at the main menu.
2. Run Control-No-Encounter-Starts.exe. Wait for ENABLED.
3. Return to Control and load an already-unlocked area for RR testing.
4. Keep the helper open. Press Enter in its window to restore encounter starts.
   Restarting Control also restores the original code.

This test blocks three encounter-start entry points plus preplaced encounter
activation, before their start events/state transitions. It does not delete
already loaded enemies, stop an already-running wave, unlock areas, or complete
missions. Scripted spawns outside these encounter systems may still occur.
Combat-dependent doors/objectives may remain blocked. Turn it off and reload
when returning to normal gameplay. Game autosaves can still record world state;
the helper itself does not edit saves or game files.

Validated by static analysis of the supplied executable and Windows memory-patch
self-tests. In-game spawn coverage remains unverified. Use this as a test build.
