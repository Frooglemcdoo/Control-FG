# Installation — Control FG v1.0.0

## Requirements

- **Control on Steam**, currently verified against Steam build `21225456`.
- **DirectX 12** (`Control_DX12.exe`). The mod does not target the DX11 executable.
- An NVIDIA RTX GPU with DLSS Frame Generation support.
- A current NVIDIA display driver.
- Windows 10/11. If DLSS Frame Generation is unavailable, verify that **Hardware-accelerated GPU scheduling (HAGS)** is enabled in Windows.

## Manual install (recommended for Nexus/GitHub release)

1. Close Control.
2. In Steam, right-click **Control** → **Properties** → **Installed Files** → **Browse**.
3. Open the folder that contains `Control_DX12.exe`.
4. Extract the Control FG release ZIP.
5. Copy these items directly into the Control game folder:

```text
Control\
├─ Control_DX12.exe
├─ dxgi.dll                    <- Control FG
└─ ControlFGStreamline\        <- Control FG
   ├─ sl.interposer.dll
   ├─ sl.common.dll
   ├─ sl.pcl.dll
   ├─ sl.reflex.dll
   ├─ sl.dlss_g.dll
   └─ nvngx_dlssg.dll
```

6. Launch Control in **DirectX 12** mode.
7. The mod window intentionally starts hidden. Press **F10** to open or close the Control FG overlay.
8. Choose the desired Frame Generation mode. Changes apply automatically and settings are saved automatically.

## Using the overlay

**Fixed modes:** 2x means one generated frame plus one rendered frame; 3x means two generated frames plus one rendered frame; and so on through 6x on supported hardware.

**Dynamic:** NVIDIA Dynamic Multi Frame Generation varies the generated-frame count to approach the selected output target. **Auto** follows the refresh rate of the monitor containing the game. **Manual** enables the 30–1000 FPS target slider.

**RTX 40-series:** only **Off** and **2x** are intentionally available. Dynamic and 3x–6x are greyed out.

## Updating Control FG

1. Close Control.
2. Replace `dxgi.dll` and the entire `ControlFGStreamline` folder with the new release files.
3. Your normal preferences are stored separately at `%LOCALAPPDATA%\ControlFG\settings.ini`, so an update does not require resetting them.

If you previously tested the abandoned v0.8.30 latency build, the obsolete `ControlFGPresentMon` folder can be deleted; current builds do not use PresentMon.

## Uninstall

Close Control, then remove the Control FG `dxgi.dll` and the `ControlFGStreamline` folder from the game directory. Do **not** delete a `dxgi.dll` unless you know it belongs to Control FG; other mods may use the same filename.

Optional cleanup:

- `%LOCALAPPDATA%\ControlFG\` — saved Control FG settings.
- `%LOCALAPPDATA%\ControlFGProbe\` — diagnostic logs.

Steam game files themselves are not replaced by the manual installation.
