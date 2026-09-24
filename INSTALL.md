# Install — Control FG v2.1.1

One deployment package supports the validated **Steam, Epic Games Store, and GOG** DX12 builds. Download `Control-FG-v2.1.1.zip`; `Control-FG-v2.1.1-Source.zip` contains source and requires compilation.

## Find your Control installation

Close Control first. Locate the folder containing `Control_DX12.exe`:

| Storefront | Open the folder | Example location |
| --- | --- | --- |
| Steam | Library → right-click Control → Manage → Browse local files | `C:\Program Files (x86)\Steam\steamapps\common\Control` |
| Epic Games Store | Library → Control's three-dot menu → Manage → folder icon beside Installation | `C:\Program Files\Epic Games\Control` |
| GOG Galaxy | Control → menu beside Play → Manage installation → Show folder | `C:\Program Files (x86)\GOG Galaxy\Games\Control` |

Custom libraries and GOG offline installations may use another location. Always check for `Control_DX12.exe` rather than relying on the example path.

## Install or update

1. Extract the deployment ZIP.
2. Copy `dxgi.dll` and the **complete `ControlFGStreamline` folder** beside `Control_DX12.exe`.
3. When updating, replace the previous Control FG files together. Do not mix DLLs from different releases. No original game EXE or engine DLL is replaced.
4. **GOG users: disable the Galaxy overlay for Control before launching**, as described below.
5. Launch Control in **DirectX 12** mode; select DX12 if the launcher asks.
6. Open the overlay with **F10** or your saved custom shortcut.

The header cog opens Options, including shortcut rebinding and optional experimental RTX 40-series MFG. Enabling experimental MFG requires restarting the game. The RR cog opens reflection clamp settings (25–75, default 60). RR Model F is the default, with live E/F selection.

Settings are preserved in `%LOCALAPPDATA%\ControlFG\settings.ini`.

## GOG Galaxy: disable the overlay for Control

Galaxy's overlay can cause input/focus stalls and make Control FG or other overlays disappear.

1. Close Control. Open its page in **GOG Galaxy**.
2. Click the menu beside **Play** → **Manage installation** → **Configure**.
3. Open **Features**. If shown, clear **Use default settings** to enable per-game changes.
4. Uncheck **Overlay** / **Access GOG GALAXY features in-game**.
5. Save/confirm if prompted and relaunch Control in DX12 mode.

If the per-game control is unavailable, use Galaxy **Settings → Game features → Overlay** to disable it globally. That setting affects all games. Restart the game after changing either setting. Control FG's F10 overlay still works.

GOG's support instructions for the per-game overlay option: [GOG Support](https://support.gog.com/hc/en-us/articles/360013153758-STAR-WARS-Episode-I-Racer-Stuttering-when-moving-the-camera).

## Compatibility

Supported targets are Steam build `21225456`, Epic binary version `0.0.518.2177`, and the verified GOG EXE identity beginning `57A8912F`. The full matching game-file combinations are listed in `target-manifest.json` in the source. Unknown or mismatched builds are rejected. DX11 is not supported.

Keep the bundled runtimes in `ControlFGStreamline`; do not install loose DLSS runtime DLLs beside the game. ReShade and RenoDX are not required. If another mod already owns `dxgi.dll`, resolve that filename conflict before installation. Read [KNOWN_ISSUES.md](KNOWN_ISSUES.md), including the RR indirect diffuse lighting workaround.

## Remove Control FG

Close Control and remove **Control FG's** `dxgi.dll` and `ControlFGStreamline` folder. Do not remove a DLL belonging to another mod. Settings can remain for a later reinstall.

## Build from source

Run `Build.cmd` on Windows. A successful build also packages `release/Control-FG-v2.1.1.zip`; `Make-DropIn.cmd` can regenerate it. Install from that ZIP using the steps above. `Install.cmd` also supports a source-tree installation into the selected Steam, Epic, or GOG directory. See [BUILDING.md](BUILDING.md).
