# Nexus Mods publishing copy

## Mod name

**Control FG - DLSS Frame Generation 2x-6x + Dynamic MFG**

## Short description

Adds engine-aware NVIDIA DLSS Frame Generation to Control (DX12), including fixed 2x–6x MFG, Dynamic MFG, live HDR handling, persistent settings, and a Control-styled F10 overlay.

## Main description

### Control FG

Control FG brings NVIDIA DLSS Frame Generation to the DirectX 12 version of **Control** with a native-looking in-game configuration overlay.

Unlike a generic FG translation layer, Control FG is built specifically around **Control's renderer**. Control does not provide an existing native Frame Generation path for the mod to simply translate. Instead, the mod reconstructs the inputs FG needs directly from the game: renderer frame boundaries, depth and motion vectors, camera data, jitter/reset state, a real pre-UI scene surface, HDR state, and presentation timing.

That makes Control FG less portable than broad compatibility tools, but gives it a much deeper, game-aware integration in a title that never originally shipped with Frame Generation.

### What makes this different

Universal tools such as OptiScaler-style translation layers or ReShade add-ons are designed first for compatibility across many games. They are strongest when a game already exposes the standardized resources, callbacks, and timing needed by an upscaler or Frame Generation API.

Control FG takes the opposite approach. It identifies and feeds the same kinds of engine data a native FG implementation would normally receive:

- Control's actual renderer frame boundaries.
- Depth and motion-vector resources from the game's existing DLSS path.
- Camera transform, field of view, jitter, and reset/discontinuity state.
- A genuine pre-UI / HUD-less scene surface for cleaner generated frames.
- HDR state and live presentation transitions.
- Game-specific synchronization and presentation behavior.

This is not meant to replace OptiScaler, ReShade, or other broad graphics frameworks—the design goals are different. Those projects prioritize **wide compatibility and reusable interception**; Control FG prioritizes **deep integration with Control itself**.

### Features

- Fixed **2x, 3x, 4x, 5x and 6x** Frame Generation / Multi Frame Generation on supported hardware.
- **Dynamic MFG** that automatically changes the multiplier to approach a target output frame rate.
- Dynamic **Auto** target based on the game monitor's refresh rate, or a **Manual 30–1000 FPS** target slider.
- **HDR support**, including switching HDR off and back on during gameplay.
- **F10 Control-style overlay** with mode, effective multiplier, current FPS, HDR state and GPU maximum.
- Overlay starts hidden and settings are saved automatically.
- **RTX 40-series:** intentionally limited to **Off + 2x**; higher MFG modes and Dynamic are greyed out.

### Requirements

- Control on **Steam** — currently verified against build **21225456**.
- Run the game in **DirectX 12** mode.
- NVIDIA RTX GPU with DLSS Frame Generation support.
- Current NVIDIA driver. If Frame Generation is unavailable, verify Hardware-accelerated GPU scheduling (HAGS) is enabled.

### Installation

1. Open Steam → Control → Properties → Installed Files → Browse.
2. Close Control if it is running.
3. Extract the mod ZIP.
4. Copy **`dxgi.dll`** and the **`ControlFGStreamline`** folder next to **`Control_DX12.exe`**.
5. Start Control in DX12.
6. Press **F10** to open the overlay.

No original Control executable or renderer file is replaced.

### Usage

Choose a fixed multiplier for predictable output, or choose **Dynamic** to let NVIDIA Dynamic MFG vary the generated-frame count toward your target FPS. Auto uses the refresh rate of the display containing Control; Manual exposes the target FPS slider.

### Updating

Close the game and overwrite the old Control FG `dxgi.dll` and `ControlFGStreamline` folder with the new release. Your preferences are stored in `%LOCALAPPDATA%\ControlFG\settings.ini`.

### Uninstall

Close Control and remove Control FG's `dxgi.dll` and `ControlFGStreamline` folder. Only remove `dxgi.dll` if you know it came from Control FG—other graphics mods can use the same filename.

### Compatibility / known limitations

- Steam DX12 build 21225456 is the currently verified target.
- DX11 is not supported.
- GOG/Epic builds are not yet claimed compatible.
- Another mod that uses its own `dxgi.dll` in the Control directory can conflict. Automatic proxy chaining is not supported in the first release.
- Very high graphics/ray-tracing workloads still need enough base-game performance and GPU headroom for good FG results.

### Troubleshooting

If F10 does not open the overlay, first confirm you launched DX12 and that `dxgi.dll` and `ControlFGStreamline` are beside `Control_DX12.exe`. If modes show WAITING or `--`, enter normal gameplay, update the NVIDIA driver, verify HAGS, and confirm the mode is supported by the GPU. For crashes, temporarily remove Control FG and verify the game works vanilla before reinstalling. A complete troubleshooting guide is included with the download.

### Credits / disclaimer

Uses **NVIDIA Streamline 2.14.1**. Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, Valve, Nexus Mods, OptiScaler, or ReShade. *Control* and related trademarks/assets belong to their respective owners.
