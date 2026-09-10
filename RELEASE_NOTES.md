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

## Verified target

**Control — Steam DX12 — Steam build 21225456**

The first release does not claim compatibility with DX11, GOG/Epic builds, later Control patches, or other wrappers that already install their own `dxgi.dll` beside `Control_DX12.exe`.

The stabilized generation runtime is based on the v0.8.26 validation baseline. Later pre-release versions focused on user interface, persistence, policy and release packaging rather than changing the core FG path.
