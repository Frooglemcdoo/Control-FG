# Control FG

![Control FG](assets/control_fg_logo.png)

**Control FG** adds NVIDIA DLSS Frame Generation to the DirectX 12 version of Remedy Entertainment's *Control*, including fixed Frame Generation and Multi Frame Generation modes, native Dynamic Multi Frame Generation, live HDR handling, persistent settings, and an in-game overlay styled to fit Control's menu aesthetic.

Unlike a generic FG translation layer, Control FG is **game-specific and engine-aware**. *Control* does not expose a native Frame Generation integration for the mod to translate, so Control FG reconstructs the inputs FG needs directly from the game's renderer: frame boundaries, depth and motion-vector resources, camera data, jitter/reset state, pre-UI scene color, HDR state, and presentation timing. That makes the project less portable than broad compatibility tools, but allows a much deeper integration with *Control* itself.

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

## Why Control FG is different

Tools such as universal upscaler/Frame Generation translators and ReShade-style add-ons are designed first for portability across many games. They typically work at a graphics/API interception layer and are strongest when a game already exposes the standardized resources and timing needed by an upscaler or Frame Generation API.

Control FG takes the opposite approach: it is **deeply integrated with one game's renderer**. Because *Control* shipped without a native FG source path, the mod identifies and reconstructs the data that a native implementation would normally provide, including:

- Control's actual renderer frame boundaries.
- Depth and motion-vector resources used by the game's existing DLSS path.
- Camera transform, field of view, jitter, and reset/discontinuity state.
- A genuine pre-UI / HUD-less scene surface for cleaner generated frames.
- HDR state and presentation transitions.
- Game-specific synchronization and presentation behavior.

This is not intended as a replacement for broad tools such as OptiScaler or ReShade. The design goals are different: those projects prioritize **wide compatibility and reusable interception**, while Control FG prioritizes **deep, game-aware integration** in a title that did not originally ship with Frame Generation.

The long-term architecture is being separated into a reusable FG backend layer and a game-specific semantic-capture layer. That makes it possible to investigate additional backends while preserving the higher-quality game data discovered specifically for *Control*.

## Hardware support

Control FG relies on NVIDIA DLSS Frame Generation support. On **GeForce RTX 40-series**, the mod intentionally exposes only **Off and 2x**. On hardware that reports Multi Frame Generation support, the overlay exposes modes up to the supported maximum, currently capped by the UI at **6x**, plus Dynamic MFG when the driver reports it available.

A current NVIDIA driver is strongly recommended. Hardware-accelerated GPU scheduling (HAGS) should be enabled if DLSS Frame Generation is unavailable on an otherwise supported system.

## Install

For the prebuilt Nexus/GitHub release, see [INSTALL.md](INSTALL.md). The short version is: copy `dxgi.dll` and the `ControlFGStreamline` folder beside `Control_DX12.exe`, launch Control in DX12 mode, and press **F10**.

## Build from source

See [BUILDING.md](BUILDING.md). `Build.cmd` downloads and hash-verifies the pinned official Streamline 2.14.1 SDK, compiles the x64 DXGI proxy, runs ABI/export/smoke checks, and creates the release ZIPs.



## Screenshots

### Dynamic Multi Frame Generation

![Dynamic MFG](screenshots/hero-dynamic-5x.png)

Dynamic Multi Frame Generation automatically adjusts the generated-frame count toward your target output FPS.

### Fixed 6× Multi Frame Generation

![6x FG](screenshots/fixed-6x.png)

Up to 6× Multi Frame Generation on supported hardware.

### Fixed 4× Multi Frame Generation

![4x FG](screenshots/fixed-4x.png)

Selectable fixed multipliers from 2× through 6×.

### RTX 40-Series Compatible 2×

![2x FG](screenshots/fixed-2x.png)

GeForce RTX 40-series GPUs are intentionally limited to Off and 2×.


## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md). The most common issues are launching the DX11 executable, using an unsupported game build, an outdated NVIDIA driver/HAGS configuration, or another mod already occupying `dxgi.dll` in the Control directory.

## Current compatibility

This release is **build-locked to the Steam DX12 version listed above**. GOG/Epic builds and later Control patches are not claimed compatible until tested. Other wrappers that install their own `dxgi.dll` in the same directory are not supported by the first release.

## Project status / roadmap

Frame Generation is Mod #1. After this release is stable, planned work moves into a separate DLSS Super Resolution module with selectable runtime/model families, followed by Ray Reconstruction research and integration. See [ROADMAP.md](ROADMAP.md).

## Credits and legal

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, or Valve. *Control* and its trademarks/assets belong to their respective owners. NVIDIA Streamline is redistributed under its upstream license; see [legal/STREAMLINE-LICENSE.txt](legal/STREAMLINE-LICENSE.txt) and [legal/THIRD-PARTY-NOTICES.md](legal/THIRD-PARTY-NOTICES.md).

No project-wide open-source license has been selected for Control FG yet. Until one is added by the project owner, normal copyright rules apply to the original Control FG source code.
