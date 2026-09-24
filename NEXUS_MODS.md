# Nexus Mods publishing copy — v2.1.1

## Mod name

**Control FG - DLSS Frame Generation 2x-6x + Dynamic MFG**

## Short description

Adds engine-aware NVIDIA DLSS Frame Generation to Control (DX12), including fixed 2x–6x MFG, Dynamic MFG, live HDR handling, stable HUD/UI separation, persistent settings, and a Control-styled F10 overlay.

## Main description

**v2.1.1 now supports Steam, Epic Games Store, and GOG in one download. GOG users must disable Galaxy's in-game overlay for Control; see the steps below.**

### Control FG

Control FG brings NVIDIA DLSS Frame Generation and Ray Reconstruction to supported Steam, Epic Games Store, and GOG DirectX 12 builds of **Control**, with a native-looking in-game configuration overlay.

Unlike a generic FG translation layer, Control FG is built specifically around **Control's renderer**. Control does not provide an existing native Frame Generation path for the mod to simply translate. Instead, the mod reconstructs the inputs FG needs directly from the game: renderer frame boundaries, depth and motion vectors, camera data, jitter/reset state, a real pre-UI scene surface, HDR state, and presentation timing.

That makes Control FG less portable than broad compatibility tools, but gives it a much deeper, game-aware integration in a title that never originally shipped with Frame Generation.

### Stable HUD/UI — one of Control FG's biggest advantages

Control FG captures Control's full-resolution scene **immediately before the game draws its HUD and UI**. The Frame Generation backend therefore receives a true HUD-less scene instead of having to treat objective text, health/energy bars, prompts, icons, and menus as moving scene content.

This is a major difference from screen-space Frame Generation solutions such as **Lossless Scaling**, which only see the final composited image, and from generic injection paths such as **OptiFG** when a true HUD-less resource is not available. Those broader solutions may need to infer or heuristically reconstruct the HUD-less image. Control FG has a build-locked point inside Control's renderer where the actual pre-UI scene is available directly.

In current Control testing this produces a **very stable HUD and UI during camera movement**, without the large UI smear/ghost trails that can appear when the HUD is included in Frame Generation.

### What makes this different

Control FG identifies and feeds the same kinds of engine data a native FG implementation would normally receive:

- Control's actual renderer frame boundaries.
- Depth and motion-vector resources from the game's existing DLSS path.
- Camera transform, field of view, jitter, and reset/discontinuity state.
- A genuine pre-UI / HUD-less scene surface.
- HDR state and live presentation transitions.
- Game-specific synchronization and presentation behavior.

This is not meant to replace OptiScaler, ReShade, Lossless Scaling, or other broad graphics frameworks—the design goals are different. Those projects prioritize **wide compatibility and reusable interception**; Control FG prioritizes **deep integration with Control itself**.

### Features

- Fixed **2x, 3x, 4x, 5x and 6x** Frame Generation / Multi Frame Generation on supported NVIDIA hardware.
- **Dynamic MFG** that automatically changes the multiplier to approach a target output frame rate.
- Dynamic **Auto** target based on the game monitor's refresh rate, or a **Manual 30–1000 FPS** target slider.
- **HDR support**, including switching HDR off and back on during gameplay.
- **F10 Control-style overlay** with mode, effective multiplier, current FPS, HDR state and GPU maximum.
- **Stable HUD/UI handling** using a real Control pre-UI scene capture.
- Overlay starts hidden and settings are saved automatically.
- **RTX 40-series:** Off + 2x by default; experimental Multi Frame Generation is available as an opt-in in Options (restart required), subject to runtime capability checks.
- **DLSS Ray Reconstruction:** Model F by default, with live E/F selection and a dedicated reflection clamp settings page.

### FSR Frame Generation expansion — in development

Control FG's AMD FSR Frame Generation backend is being developed using the same game-specific depth, motion-vector, camera, timing, and HUD-less inputs.

The target hardware layout is:

- **RTX 20-series:** FSR Frame Generation.
- **RTX 30-series:** FSR Frame Generation.
- **RTX 40-series:** selectable DLSS Frame Generation or FSR Frame Generation.
- **RTX 50-series:** selectable DLSS Frame Generation / Multi Frame Generation or FSR Frame Generation.

The goal is to expose the backend choice directly in the Control FG overlay. This is still development work and is not part of the current public Nexus release until the unified runtime selector is complete.

### Requirements

- A validated Control build from **Steam (21225456)**, **Epic Games Store (0.0.518.2177)**, or **GOG (EXE hash beginning 57A8912F)**.
- Run the game in **DirectX 12** mode.
- Current public release: NVIDIA RTX GPU with DLSS Frame Generation support.
- Current NVIDIA driver. If Frame Generation is unavailable, verify Hardware-accelerated GPU scheduling (HAGS) is enabled.

### Installation — Steam, Epic Games Store, and GOG

1. Close Control and download `Control-FG-v2.1.1.zip`.
2. Open the folder containing `Control_DX12.exe`:
   - **Steam:** Library → right-click Control → Manage → Browse local files.
   - **Epic:** Library → Control's three-dot menu → Manage → folder icon beside Installation.
   - **GOG Galaxy:** Control → menu beside Play → Manage installation → Show folder. For offline installations, use your chosen game folder.
3. Extract the mod ZIP and copy **`dxgi.dll`** and the **complete `ControlFGStreamline` folder** beside **`Control_DX12.exe`**.
4. For GOG, disable Galaxy's overlay using the steps below.
5. Start Control in **DX12** and press **F10** (or your saved shortcut) to open the overlay.

No original Control executable or renderer file is replaced.

### GOG Galaxy overlay caveat

Galaxy's overlay can cause input/focus stalls and make Control FG or other overlays disappear. Close Control, open its Galaxy page, and select **menu beside Play → Manage installation → Configure → Features**. Clear **Use default settings** if shown, then uncheck **Overlay / Access GOG GALAXY features in-game**. Save/confirm if prompted and restart Control.

If the per-game control is unavailable, **Settings → Game features → Overlay** disables it globally. Control FG's own F10 overlay remains available.

### Usage

Choose a fixed multiplier for predictable output, or choose **Dynamic** to let NVIDIA Dynamic MFG vary the generated-frame count toward your target FPS. Auto uses the refresh rate of the display containing Control; Manual exposes the target FPS slider.

### Updating

Close the game and overwrite the old Control FG `dxgi.dll` and `ControlFGStreamline` folder with the new release. Your preferences are stored in `%LOCALAPPDATA%\ControlFG\settings.ini`.

### Uninstall

Close Control and remove Control FG's `dxgi.dll` and `ControlFGStreamline` folder. Only remove `dxgi.dll` if you know it came from Control FG—other graphics mods can use the same filename.

### Compatibility / known limitations

- Validated Steam, Epic Games Store, and GOG DX12 builds are supported in one package. Complete matching game-file identities are required.
- DX11 is not supported.
- Disable GOG Galaxy's in-game overlay for Control. Unknown or mismatched builds and unvalidated game updates are not supported.
- RR with Ray Traced Indirect Diffuse Lighting can still cause crawling/noisy streaks; disable that game setting or RR as a workaround. The reflection clamp does not resolve this artifact.
- Another mod that uses its own `dxgi.dll` in the Control directory can conflict. Automatic proxy chaining is not supported in the first release.
- Very high graphics/ray-tracing workloads still need enough base-game performance and GPU headroom for good FG results.

### Troubleshooting

If F10 does not open the overlay, first confirm you launched DX12 and that `dxgi.dll` and `ControlFGStreamline` are beside `Control_DX12.exe`. If modes show WAITING or `--`, enter normal gameplay, update the NVIDIA driver, verify HAGS, and confirm the mode is supported by the GPU. For crashes, temporarily remove Control FG and verify the game works vanilla before reinstalling. A complete troubleshooting guide is included with the download.

### Credits / disclaimer

A massive shoutout to **HotKnives** for lending his machine and helping with QA for the recent releases and bug fixes!

Uses **NVIDIA Streamline 2.14.1** in the current public release. Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, AMD, Valve, Nexus Mods, OptiScaler, ReShade, or Lossless Scaling. *Control* and related trademarks/assets belong to their respective owners.
