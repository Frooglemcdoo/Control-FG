# Control The Oldest House: Visual and Performance Overhaul — R257

Copy the full package beside `Control_DX12.exe`, preserving the folder structure.
Launch normally through Steam. No F6 or diagnostic launcher is needed.

## Installation layout

| Location | Contents |
| --- | --- |
| Game directory | `dxgi.dll`, `dstorage.dll`, `dstoragecore.dll` |
| `ControlTOH/Shaders` | 17 runtime shader sidecars |
| `ControlTOH/Tools` | Restart helper, log collectors, legacy cleanup helper |
| `ControlTOH/Diagnostics` | Optional capture scripts, diagnostic launchers, DirectStorage validation folder |
| `ControlTOH` | Readme and build notes |
| `ControlFGStreamline` | Existing Streamline runtime location retained |

Shader loading, restart preparation and in-game diagnostic commands resolve the new locations. Shader filenames and contents are unchanged. The DirectStorage DLLs remain beside the executable because the validated SDK wrapper resolves its core runtime there. Internal settings, environment-variable names and log directories keep their existing identities so upgrades retain preferences and collection compatibility.

## Upgrade cleanup

Close Control, copy the complete package, then run `ControlTOH/Tools/Organize-Existing-Install.cmd`. It archives recognized legacy root shader/helper/collector files and old validator files under a unique `ControlTOH/LegacyBackup` directory. It never overwrites a backup or deletes files. Unknown files, generic readmes, game files, the three root DLLs and Streamline are left alone. Existing archives and unrelated old test launchers are not removed automatically.

## Preserved behavior

The R256 native DirectStorage checkbox, restart confirmation and hidden streaming budget are retained. Accepted R254 GPU transport, prewarming and all-texture upgrade eligibility remain unchanged. DirectStorage uses the automatic 4 ms budget and 16 pinned partitions on the validated Steam build. Optional timing and texture observers remain off by default. This build changes installation paths and branding, not rendering or streaming algorithms.

## Validation

The Windows workflow checks that all 17 sidecars and every relocated runtime helper are packaged, exercises cleanup twice in an installation path containing spaces, verifies backups and preservation of game/foreign files, and runs the existing restart-helper process tests using the packaged helper's new location. Existing rendering, storage, menu and startup checks remain enabled.

Live acceptance: launch with the complete package, verify the accepted visuals and texture-loading route, then test DirectStorage Cancel and confirmed Off/On restart. Actual game rendering and Steam relaunch still need a live check.
