# Control FG

![Control FG](assets/control_fg_logo.png)

**Control FG** adds NVIDIA DLSS Frame Generation, Multi Frame Generation, Dynamic MFG, and DLSS Ray Reconstruction to the DirectX 12 version of Remedy Entertainment's *Control*.

Unlike a generic graphics injection layer, Control FG is **game-specific and engine-aware**. *Control* does not expose a native Frame Generation integration for the mod to translate, so the project reconstructs the inputs FG and RR need directly from the game's renderer: frame boundaries, depth and motion vectors, camera/jitter state, pre-UI scene color, ray-tracing resources, HDR state, and presentation timing.

> **Current release:** [v2.1.0](https://github.com/Frooglemcdoo/Control-FG/releases/tag/v2.1.0)  
> **Verified game target:** Control on Steam, DX12, Steam build `21225456`  
> **Streamline:** NVIDIA Streamline `2.14.1`

## Video Demonstration

[![Control FG – App and Overlay Demonstration](https://img.youtube.com/vi/aP7UeCSx00c/maxresdefault.jpg)](https://youtu.be/aP7UeCSx00c)

[Watch Control FG – App and Overlay Demonstration on YouTube](https://youtu.be/aP7UeCSx00c)

## What's new in v2.1.0

- **Experimental RTX 40-series Multi Frame Generation**, available as an opt-in setting in Options. Disabled by default; restart Control after enabling it.
- A dedicated **Ray Reconstruction settings page**, with a reflection clamp slider from **25 to 75**, default **60**, and a **Reset to default** button.
- **Overlay shortcut rebinding** in Options.

## Fixes and improvements

- Reworked Frame Generation synchronization and resource handling to address flickering, ghosting, and stability problems.
- Improved HDR/SDR transition handling and Frame Generation recovery.
- Improved HUD/UI handling on generated frames.
- Improved GPU command and resource lifetime management during resizing, resource replacement, and shutdown.
- Added safeguards for stale frames, queue changes, and invalid device/resource combinations.
- Improved RR Model F projection validation and hit-distance handling.
- Refined RR reflection clamping to preserve more natural skin and material appearance.
- Cleaned up the overlay and removed the HDR restart footer.

## Known issues

**RR indirect diffuse lighting:** blinds can show crawling/noisy streaks when Ray Reconstruction and **Ray Traced Indirect Diffuse Lighting** are enabled together. Reported on both RTX 40- and 50-series GPUs; Models E and F behave similarly. **Workaround:** disable Ray Traced Indirect Diffuse Lighting in the game, or disable RR in the overlay.

Periodic frame-time spikes with the overlay visible have also been reported on an RTX 4070. Close the overlay during gameplay if affected. If an HDR transition causes corruption, restart Control with the desired HDR setting. See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for support details.

## DLSS Ray Reconstruction

Ray Reconstruction runs at Control's native ray-tracing denoising boundary rather than being applied over the already-composited final image.

When RR is enabled, Control FG bypasses Control's native denoiser for the RR path and supplies NVIDIA Ray Reconstruction with game-native renderer data including:

- depth and motion vectors;
- camera and jitter state;
- ray-tracing resources;
- reset/discontinuity state;
- HDR and presentation context.

This keeps RR on the correct side of the renderer pipeline, avoids a double-denoise path, and lets it coexist with Control's DLSS modes, ray tracing, HDR, and Frame Generation.

### RR performance expectations

Extensive testing shows the largest RR cost at **4K + DLAA**:

- **4K + DLAA:** roughly **20–30%** lower performance with RR enabled.
- **1440p / 1080p:** typically around **2–10%**, often difficult to notice depending on the scene.
- **4K + DLSS Quality:** the RR cost is substantially less noticeable than 4K DLAA.

The 4K DLAA hit was reproduced across repeated RR toggles, resolution changes, DLSS modes, and RT configurations and currently appears to be a real workload cost rather than an obvious mod-side state or recovery bug.

## Frame Generation

- **Off / 2x / 3x / 4x / 5x / 6x** fixed Frame Generation modes on supported hardware.
- **Native Dynamic MFG** with automatic monitor-refresh targeting or a manual 30–1000 FPS target.
- Runtime pacing using NVIDIA Reflex and PCL `SimulationStart`.
- **RTX 40-series:** Off and 2x by default; optional experimental MFG through Options (restart required).
- HDR/SDR transition recovery; see known issues if a transition causes corruption.
- Persistent settings stored in `%LOCALAPPDATA%\ControlFG\settings.ini`.

### HDR/SDR recovery

When Windows changes HDR state while FG is active, Control FG commits DLSS-G off, frees its feature resources, lets the display-domain transition settle, requires fresh tagged frames, and then cleanly rearms FG.

The recovery also covers transitions where Control does not call `ResizeBuffers` and suppresses delayed duplicate HDR notifications so the same transition does not start a second teardown.

## Stable HUD and UI under Frame Generation

Control FG captures the full-resolution scene immediately before Control draws its HUD/UI and handles the UI separately.

Generated frames therefore do not have to interpret objective text, health/energy elements, prompts, icons, and menus as moving world geometry when the camera moves. This produces a substantially more stable HUD during generated frames while allowing the scene itself to interpolate normally.

## Why Control FG is different

The project prioritizes **deep integration with Control's renderer** rather than broad game compatibility.

The mod identifies and reconstructs data a native implementation would normally provide:

- Control's actual renderer frame boundaries;
- depth and motion-vector resources used by the existing DLSS path;
- camera transform, field of view, jitter, and reset state;
- a genuine pre-UI / HUD-less scene surface;
- ray-tracing resources used by Ray Reconstruction;
- HDR and presentation transitions;
- game-specific synchronization and presentation behavior.

That game-specific data is shared across the FG and RR integrations instead of treating either feature as a final-image effect.

## Hardware support

Control FG's current public release relies on NVIDIA DLSS Frame Generation support.

On **GeForce RTX 40-series**, **Off and 2x** remain the default. Enable **experimental RTX 40-series MFG** in Options and restart Control to use the extended path. Availability remains subject to runtime capability checks. On hardware that reports Multi Frame Generation support, the overlay exposes modes up to the supported maximum, currently capped by the UI at **6x**, plus Dynamic MFG when supported.

A current NVIDIA driver is strongly recommended. Hardware-accelerated GPU scheduling (HAGS) should be enabled if DLSS Frame Generation is unavailable on otherwise supported hardware.

## Install

See [INSTALL.md](INSTALL.md).

The short version:

1. Copy `dxgi.dll` beside `Control_DX12.exe`.
2. Copy the complete `ControlFGStreamline` folder beside it.
3. Launch Control in DX12 mode.
4. Press **F10**, or your saved custom shortcut. The header cog opens Options; the cog beside RR opens its settings.

## Screenshots

### Dynamic Multi Frame Generation

![Dynamic MFG](screenshots/hero-dynamic-5x.png)

Dynamic MFG automatically adjusts the generated-frame count toward the selected output target.

### Fixed 6× Multi Frame Generation

![6x FG](screenshots/fixed-6x.png)

Up to 6× Multi Frame Generation on supported hardware.

### Fixed 4× Multi Frame Generation

![4x FG](screenshots/fixed-4x.png)

Selectable fixed multipliers from 2× through 6×.

### RTX 40-Series 2×

![2x FG](screenshots/fixed-2x.png)

RTX 40-series defaults to Off and 2×. v2.1.0 adds optional experimental MFG in Options; restart after enabling it.

## Troubleshooting and logs

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

If you need to report an issue, reproduce it once and run `Collect-ControlFG-Compact-Logs.cmd`, then attach the generated ZIP.

## Current compatibility

This release is **build-locked to Control on Steam, DX12, Steam build `21225456`**. DX11 is not supported. Other storefront builds and later game patches are not claimed compatible until tested.

## Roadmap

The next major development target is **DLSS 5 integration**.

After that, planned research includes extending Frame Generation / Multi Frame Generation support to **RTX 20-series and RTX 30-series** hardware. See [ROADMAP.md](ROADMAP.md).

## Credits and legal

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, AMD, or Valve. *Control* and its trademarks/assets belong to their respective owners.

NVIDIA Streamline is redistributed under its upstream license; see [legal/STREAMLINE-LICENSE.txt](legal/STREAMLINE-LICENSE.txt) and [legal/THIRD-PARTY-NOTICES.md](legal/THIRD-PARTY-NOTICES.md).

No project-wide open-source license has been selected for Control FG yet. Until one is added by the project owner, normal copyright rules apply to the original Control FG source code.
