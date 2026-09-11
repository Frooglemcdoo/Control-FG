# Control FG

![Control FG](assets/control_fg_logo.svg)

**Control FG** adds NVIDIA DLSS Frame Generation to the DirectX 12 version of Remedy Entertainment's *Control*, including fixed Frame Generation and Multi Frame Generation modes, native Dynamic Multi Frame Generation, live HDR handling, persistent settings, and an in-game overlay styled to fit Control's menu aesthetic.

> **Current release:** v1.0.0  
> **Verified game target:** Control on Steam, DX12, Steam build `21225456`  
> **Streamline:** NVIDIA Streamline `2.14.1`

## Features

- **Off / 2x / 3x / 4x / 5x / 6x** fixed Frame Generation modes on supported hardware.
- **Native Dynamic MFG** with Auto monitor-refresh targeting or a manual 30–1000 FPS target.
- **Live HDR support**, including enabling/disabling HDR during gameplay without restarting the game.
- **F10 overlay** that starts hidden and shows current mode, effective multiplier, output FPS, HDR state, and reported GPU maximum.
- **Persistent settings** stored in `%LOCALAPPDATA%\ControlFG\settings.ini`.
- **RTX 40-series safety policy:** Off and 2x are available; Dynamic and 3x–6x are disabled.
- Runtime-proven Dynamic MFG integration using Reflex pacing and the PCL `SimulationStart` marker required by the working path.

## Hardware support

Control FG relies on NVIDIA DLSS Frame Generation support. On **GeForce RTX 40-series**, the mod intentionally exposes only **Off and 2x**. On hardware that reports Multi Frame Generation support, the overlay exposes modes up to the supported maximum, currently capped by the UI at **6x**, plus Dynamic MFG when the driver reports it available.

A current NVIDIA driver is strongly recommended. Hardware-accelerated GPU scheduling (HAGS) should be enabled if DLSS Frame Generation is unavailable on an otherwise supported system.

## Install

For the prebuilt Nexus/GitHub release, see [INSTALL.md](INSTALL.md). The short version is: copy `dxgi.dll` and the `ControlFGStreamline` folder beside `Control_DX12.exe`, launch Control in DX12 mode, and press **F10**.

## Build from source

See [BUILDING.md](BUILDING.md). `Build.cmd` downloads and hash-verifies the pinned official Streamline 2.14.1 SDK, compiles the x64 DXGI proxy, runs ABI/export/smoke checks, and creates the release ZIPs.

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md). The most common issues are launching the DX11 executable, using an unsupported game build, an outdated NVIDIA driver/HAGS configuration, or another mod already occupying `dxgi.dll` in the Control directory.

## Current compatibility

This release is **build-locked to the Steam DX12 version listed above**. GOG/Epic builds and later Control patches are not claimed compatible until tested. Other wrappers that install their own `dxgi.dll` in the same directory are not supported by the first release.

## Project status / roadmap

Frame Generation is Mod #1. After this release is stable, planned work moves into a separate DLSS Super Resolution module with selectable runtime/model families, followed by Ray Reconstruction research and integration. See [ROADMAP.md](ROADMAP.md).

## Credits and legal

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, or Valve. *Control* and its trademarks/assets belong to their respective owners. NVIDIA Streamline is redistributed under its upstream license; see [legal/STREAMLINE-LICENSE.txt](legal/STREAMLINE-LICENSE.txt) and [legal/THIRD-PARTY-NOTICES.md](legal/THIRD-PARTY-NOTICES.md).

No project-wide open-source license has been selected for Control FG yet. Until one is added by the project owner, normal copyright rules apply to the original Control FG source code.
