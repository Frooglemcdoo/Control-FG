# Control FG v1.0.0

First public release of Control FG for **Control (Steam DX12)**.

## Highlights

- Fixed NVIDIA DLSS Frame Generation / Multi Frame Generation modes: **2x, 3x, 4x, 5x and 6x** on supported hardware.
- **Native Dynamic MFG** with automatic monitor-refresh targeting or a manual 30–1000 FPS output target.
- Working Dynamic pacing path using NVIDIA Reflex and PCL `SimulationStart`.
- Live SDR/HDR transitions.
- Control-styled **F10 overlay** that starts hidden.
- Persistent settings, current FPS, effective multiplier, HDR status and GPU capability display.
- RTX 40-series safety policy: **Off + 2x only**.

## Compatibility

Verified target: **Control on Steam, DX12, Steam build 21225456**.

Other storefront builds and later game patches are not claimed compatible until tested. DX11 is not supported.

## Installation

Download `Control-FG-v1.0.0.zip`, then copy `dxgi.dll` and the `ControlFGStreamline` folder next to `Control_DX12.exe`. Launch Control in DX12 and press **F10**.

See `INSTALL.md` and `TROUBLESHOOTING.md` for complete instructions.

## September 11, 2026 overlay stability hotfix

The v1.0.0 binary asset was refreshed with the tested F10 overlay stability fix. The overlay is double-buffered, avoids repeated HWND position/visibility churn, caches the Control game window, and refreshes passive status at a reduced cadence. This fixes the overlay flicker that was most visible in menus/title screens and during screen recording. The validated v0.8.26 Frame Generation/HDR/Streamline core is unchanged.

## Notes

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, or Valve.
