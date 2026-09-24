# Control FG

![Control FG](assets/control_fg_logo.png)

**Control FG** adds NVIDIA DLSS Frame Generation, Multi Frame Generation, Dynamic MFG, and DLSS Ray Reconstruction to the DirectX 12 version of Remedy Entertainment's *Control*.

Unlike a generic graphics injection layer, Control FG is **game-specific and engine-aware**. *Control* does not expose a native Frame Generation integration for the mod to translate, so the project reconstructs the inputs FG and RR need directly from the game's renderer: frame boundaries, depth and motion vectors, camera/jitter state, pre-UI scene color, ray-tracing resources, HDR state, and presentation timing.

> **Current release:** [v2.1.1](https://github.com/Frooglemcdoo/Control-FG/releases/tag/v2.1.1)
>
> **Supported storefronts:** Steam, Epic Games Store, and GOG — DirectX 12
>
> **Streamline:** NVIDIA Streamline `2.14.1`

## Huge thanks to HotKnives!

A massive shoutout to **HotKnives** for lending me his machine and helping with QA for the recent release and bug fixes. Having his hardware available and his help testing changes made a huge difference in tracking down issues and getting v2.1.0 ready. Really appreciate the time and support!

## Video Demonstration

[![Control FG – App and Overlay Demonstration](https://img.youtube.com/vi/aP7UeCSx00c/maxresdefault.jpg)](https://youtu.be/aP7UeCSx00c)

[Watch Control FG – App and Overlay Demonstration on YouTube](https://youtu.be/aP7UeCSx00c)

## What's new in v2.1.1

- **Epic Games Store and GOG are now supported**, alongside Steam, in one download.
- Validated storefront profiles retain the exact game-file checks used by the runtime and installer.
- Installation instructions now cover all three storefronts.
- **GOG users: disable the GOG Galaxy in-game overlay for Control** before playing. See the steps below.

### Included from v2.1.0

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

**GOG Galaxy overlay conflict:** Galaxy's in-game overlay can cause input/focus stalls and make Control FG or other overlays disappear. Disable it for Control using the [steps below](#gog-disable-galaxys-in-game-overlay).

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

## Install — Steam, Epic Games Store, and GOG

Download **`Control-FG-v2.1.1.zip`** from the [release assets](https://github.com/Frooglemcdoo/Control-FG/releases/tag/v2.1.1). This is the same deployment package for all three stores; the source ZIP requires compilation.

Close Control, then locate the folder containing **`Control_DX12.exe`**:

| Storefront | Find the game folder | Example path (your location may differ) |
| --- | --- | --- |
| Steam | Library → right-click Control → Manage → Browse local files | `C:\Program Files (x86)\Steam\steamapps\common\Control` |
| Epic Games Store | Library → Control's three-dot menu → Manage → folder icon beside Installation | `C:\Program Files\Epic Games\Control` |
| GOG Galaxy | Control → menu beside Play → Manage installation → Show folder | `C:\Program Files (x86)\GOG Galaxy\Games\Control` |

1. Extract the release ZIP.
2. Copy **`dxgi.dll`** and the **complete `ControlFGStreamline` folder** beside `Control_DX12.exe`. Replace your previous Control FG files together when updating.
3. For GOG, disable Galaxy's in-game overlay using the steps below.
4. Launch **Control in DirectX 12 mode**. If the launcher offers a choice, select DX12.
5. Press **F10**, or your saved custom shortcut. The header cog opens Options; the cog beside RR opens its settings.

For a GOG offline installation, use the folder you selected when installing the game. See [INSTALL.md](INSTALL.md) for updating, removal, and source-build instructions.

### GOG: disable Galaxy's in-game overlay

1. Close Control and select it in **GOG Galaxy**.
2. Open the menu beside **Play** → **Manage installation** → **Configure**.
3. Select **Features** and uncheck **Overlay** / **Access GOG GALAXY features in-game**. If shown, turn off **Use default settings** first so you can change the per-game option.
4. Save/confirm the change if prompted, then relaunch Control in DX12 mode.

If the per-game option is unavailable, Galaxy's global setting is **Settings → Game features → Overlay**; disabling it there affects all games. The game must be restarted after changing the setting. This workaround concerns Galaxy's overlay; Control FG's F10 overlay remains available.

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

**Steam, Epic Games Store, and GOG are supported** for the validated DX12 builds:

| Storefront | Validated identity |
| --- | --- |
| Steam | Build `21225456` |
| Epic Games Store | Game binary version `0.0.518.2177` |
| GOG | EXE SHA-256 begins `57A8912F`; engine DLLs match the verified Steam binaries |

The runtime and source installer require one complete, recognized combination of `Control_DX12.exe`, `d3d_rmdwin10_f.dll`, and `renderer_rmdwin10_f.dll`. Unknown or mismatched combinations are rejected. Exact identities are recorded in [target-manifest.json](target-manifest.json). DX11 and unvalidated game updates are not supported. **Disable Galaxy's in-game overlay when using the GOG version.**

## Roadmap

The next major development target is **DLSS 5 integration**.

After that, planned research includes extending Frame Generation / Multi Frame Generation support to **RTX 20-series and RTX 30-series** hardware. See [ROADMAP.md](ROADMAP.md).

## Credits and legal

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, AMD, or Valve. *Control* and its trademarks/assets belong to their respective owners.

NVIDIA Streamline is redistributed under its upstream license; see [legal/STREAMLINE-LICENSE.txt](legal/STREAMLINE-LICENSE.txt) and [legal/THIRD-PARTY-NOTICES.md](legal/THIRD-PARTY-NOTICES.md).

No project-wide open-source license has been selected for Control FG yet. Until one is added by the project owner, normal copyright rules apply to the original Control FG source code.
