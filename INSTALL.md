# Install — Control FG v2.0.0

## Requirements

- Control on Steam, DX12, verified against Steam build `21225456`
- NVIDIA RTX GPU with DLSS Frame Generation support
- Current NVIDIA driver
- Hardware-accelerated GPU scheduling (HAGS) enabled if DLSS Frame Generation is unavailable

## Install

1. Close Control.
2. Extract this ZIP.
3. Copy `dxgi.dll` and the entire `ControlFGStreamline` folder beside `Control_DX12.exe`.
4. Launch Control in DX12 mode.
5. Press **F10** to open the Control FG overlay.

The package should look like:

```text
Control\
├─ Control_DX12.exe
├─ dxgi.dll
└─ ControlFGStreamline\
   ├─ sl.interposer.dll
   ├─ sl.common.dll
   ├─ sl.pcl.dll
   ├─ sl.reflex.dll
   ├─ sl.dlss_g.dll
   ├─ nvngx_dlssg.dll
   ├─ sl.dlss_d.dll
   └─ nvngx_dlssd.dll
```

Do not place loose DLSS/RR runtime DLLs beside the game executable. Keep the bundled NVIDIA runtime files inside `ControlFGStreamline`.

## Overlay

Press **F10** to open or close the overlay.

- Frame Generation and Ray Reconstruction are controlled independently.
- RR Model **F** is the default.
- RR Model **E** remains available for comparison.
- Settings are saved automatically in `%LOCALAPPDATA%\ControlFG\settings.ini`.

## Updating

Close Control, then replace `dxgi.dll` and the entire `ControlFGStreamline` folder with the files from the newer release.

## Uninstall

Close Control and remove the Control FG `dxgi.dll` and `ControlFGStreamline` folder. Do not delete a `dxgi.dll` unless you know it belongs to Control FG.
