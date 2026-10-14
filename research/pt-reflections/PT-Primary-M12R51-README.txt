M12R51 - pause private descriptor copies with PT off; settings-reset evidence

R50 successfully registered both primary variants and produced GPU-covered PT
output. Its log also recorded 26,757,850 mirrored descriptors while PT was off
for nearly the entire session. This build pauses private descriptor mirroring
with the live PT toggle off. Native API calls still execute. Re-enabling PT
invalidates the old table epoch and requires fresh native descriptor writes
before PT can render. Existing submitted snapshots retain their storage.
Geometry capture and its retirement remain active: this is not full PT teardown.

PT-Enable.cmd now calls PT-Enable.ps1, refuses mode changes while Control runs,
backs up renderer.ini and the mod INI files, and verifies their hashes remain
unchanged after enabling. Recovery copies are under ControlFG-Settings-Backups.
It never restores or rewrites settings automatically. The runtime takes read-only
renderer.ini snapshots at startup and early presents; the collector includes them.
The cause of the native settings reset is not yet established. These snapshots
distinguish enable-command changes from game-startup changes.

Close Control, extract all files, run PT-Enable.cmd, and launch normally.
Enable native RT if needed (still required by PT); choose your preferred HDR.
Compare DLAA and Quality with PT off, then toggle PT on/off in the same scene.
Run Collect-PT-Primary-M12R51.cmd after closing the game and send its ZIP.
No ETL is required. FPS improvement and settings persistence need in-game testing.
