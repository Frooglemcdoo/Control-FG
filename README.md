# Control FG

![Control FG](assets/control_fg_logo.png)

**Control FG** adds NVIDIA DLSS Frame Generation to the DirectX 12 version of Remedy Entertainment's *Control*, including fixed Frame Generation and Multi Frame Generation modes, native Dynamic Multi Frame Generation, live HDR handling, persistent settings, and an in-game overlay styled to fit Control's menu aesthetic.

> **Current release:** v1.0.0  
> **Verified game target:** Control on Steam, DX12, Steam build `21225456`  
> **Streamline:** NVIDIA Streamline `2.14.1`

## Features

- **Off / 2x / 3x / 4x / 5x / 6x** fixed Frame Generation modes on supported hardware.
- **Native Dynamic MFG** with Auto monitor-refresh targeting or a manual 30–1000 FPS target.
- **Live HDR support**, including enabling/disabling HDR during gameplay without restarting the game.
- **F10 overlay** that starts hidden and shows current mode, effective multiplier, output FPS, HDR state, and reported GPU maximum.
- **Persistent settings** stored in `%LOCALAPPDATA%\\ControlFG\\settings.ini`.
- **RTX 40-series safety policy:** Off and 2x are available; Dynamic and 3x–6x are disabled.
- Runtime-proven Dynamic MFG integration using Reflex pacing and the PCL `SimulationStart` marker required by the working path.
