# Control FG v1.0.0 — Release Notes

Control FG v1.0.0 is the first public release of the Control DX12 Frame Generation mod.

## Included in v1.0.0

- Fixed DLSS Frame Generation / Multi Frame Generation modes from **2x through 6x** on supported hardware.
- **Dynamic Multi Frame Generation** with Auto monitor refresh targeting or a Manual 30–1000 FPS output target.
- Dynamic pacing using NVIDIA Reflex and the PCL `SimulationStart` marker used by the validated runtime path.
- Live HDR support, including HDR off/on transitions without restarting Control.
- A persistent, Control-styled **F10 overlay** showing mode, effective multiplier, current FPS, HDR state and GPU maximum.
- Settings persisted at `%LOCALAPPDATA%\ControlFG\settings.ini`.
- RTX 40-series mode policy exposing **Off + 2x** while disabling unsupported higher MFG/Dynamic choices.
- Public build/release scripts, diagnostics collection and troubleshooting documentation.

## Overlay stability hotfix

The production overlay now uses a lower-churn compositor path that eliminates the flicker reproduced most heavily on Control's title/menu screens and while screen recording.

- Overlay drawing is double-buffered and published as one completed frame.
- The 50 ms hotkey/input timer remains responsive, but expensive window and compositor operations are no longer performed every timer tick.
- Control's game HWND is cached.
- Overlay placement is checked periodically and `SetWindowPos` is called only when the calculated rectangle changes.
- Visibility changes call `ShowWindow` only on actual show/hide transitions.
- Passive runtime status is refreshed at 4 Hz while direct mouse/button/slider interaction still repaints immediately.
- New `FG_OVERLAY_CADENCE` logging records paint rate, placement changes and game-window discovery activity.

This hotfix changes only the external Win32 overlay path. The validated v0.8.26 Frame Generation, HDR10, Reflex, PCL, resource-tagging and Streamline generation baseline is unchanged.

## Verified target

**Control — Steam DX12 — Steam build 21225456**

The first release does not claim compatibility with DX11, GOG/Epic builds, later Control patches, or other wrappers that already install their own `dxgi.dll` beside `Control_DX12.exe`.

The stabilized generation runtime is based on the v0.8.26 validation baseline. Later pre-release versions focused on user interface, persistence, policy and release packaging rather than changing the core FG path.
