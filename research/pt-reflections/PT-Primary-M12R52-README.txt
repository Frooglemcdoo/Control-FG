M12R52 - persistent PT startup selection; clean RT-only startup

Close Control and extract all package files beside the installed dxgi.dll.
Do not run PT-Enable.cmd for normal updates. Your saved PT selection persists.
Open the overlay and select PT on or off, then fully close and restart Control.
The button shows RESTART while the requested mode differs from this session.
Changing your mind before restarting cancels the pending selection.

PT off after restart bypasses PT capture hooks, geometry tracking and descriptor
mirroring. PT on after restart starts fresh scene capture and enables rendering.
This is deliberately restart-only; full safe live teardown is not implemented.
Native RT remains required for PT. This build does not change native RT, HDR,
DLSS, resolution or quality settings, and includes no renderer.ini.

The overlay and optional PT-Enable.cmd / RT-Only.cmd now write the same single
[PTReflections] Enabled key in the existing ControlFG mod settings file.
Old PT marker files are ignored. No setting defaults are shipped in this ZIP.
Existing saved PT=0 starts RT-only; saved PT=1 starts PT. No saved value defaults
to RT-only. F11 remains a diagnostic rendering toggle within a PT session; it
is not the clean capture-off mode and does not alter the next-launch selection.

R51 evidence confirmed the native settings were already reset before its enable
command ran. The original reset writer remains unidentified. The scripts retain
recovery copies; startup snapshots remain read-only. No automatic restoration.

After a restart with PT off, compare DLAA and Quality in the same scene with
native RT unchanged. Then select PT on and restart before testing PT.
Run Collect-PT-Primary-M12R52.cmd after closing the game if results are needed.
Windows/WARP validation cannot establish in-game FPS: that remains unverified.
PT-on performance and lighting improvements remain unfinished. Shaders unchanged.
