# Install — Control FG v2.1.0

Close Control. Extract the release ZIP and copy `dxgi.dll` and the complete `ControlFGStreamline` folder beside `Control_DX12.exe`, replacing the previous Control FG files. Keep the files together; do not mix versions. Launch the DX12 version of Control.

For a source build, run `Build.cmd`, then `Make-DropIn.cmd`; install from the generated `release/Control-FG-v2.1.0.zip`. `Install.cmd` also supports installation from the source tree.

Open the overlay with F10 (or your saved custom shortcut). The header cog opens Options, including shortcut rebinding and optional experimental RTX 40 MFG. Enabling that option requires restarting the game.

The cog beside RR opens its settings. Reflection clamp ranges from 25 to 75; Reset to default sets 60. RR Model F is the default, with live E/F selection.

Do not place loose DLSS runtime files beside the game. ReShade and RenoDX are not required. Read KNOWN_ISSUES.md, including the RR indirect diffuse lighting workaround.
