# Control FG v2.0.0

Control FG adds NVIDIA DLSS Frame Generation, Multi Frame Generation, Dynamic MFG, and DLSS Ray Reconstruction to the DirectX 12 version of Remedy Entertainment's *Control*.

> **Current release:** v2.0.0  
> **Verified target:** Control on Steam, DX12, Steam build `21225456`  
> **Streamline:** NVIDIA Streamline `2.14.1`

## What's new in v2.0.0

- **DLSS Ray Reconstruction** integrated directly into Control's native DX12 ray-tracing path.
- NVIDIA RR runtime **310.9.1** with **Preset F** as the default and live **E/F** switching.
- Finalized **HDR/SDR Frame Generation recovery** with a hard DLSS-G resource reset and clean rearm.
- Handles HDR transitions where Control does **not** call `ResizeBuffers`.
- Suppresses delayed duplicate HDR transitions that could trigger an unnecessary second FG teardown.
- Improved **HUD/UI stability** using Control's real pre-UI scene plus separate UI recomposition for generated frames.
- Dynamic Target FPS controls are shown only while **Dynamic FG** is selected; the whole section collapses in fixed modes.
- Overlay stability/flicker cleanup.

## Ray Reconstruction

Ray Reconstruction is implemented at Control's native ray-tracing/denoising boundary rather than as a final-image post-process. When RR is enabled, Control FG bypasses Control's native denoiser for the RR path and provides NVIDIA Ray Reconstruction with game-native renderer data including depth, motion vectors, camera/jitter state, ray-tracing resources, reset state, and the current HDR/presentation context.

This keeps RR on the correct side of the renderer pipeline, avoids a double-denoise path, and allows it to coexist with Control's DLSS modes, ray tracing, HDR, and Frame Generation.

### RR performance expectations

Based on repeated testing:

- **4K + DLAA:** expect roughly a **20–30% performance hit** with RR enabled.
- **1440p / 1080p:** usually around **2–10%**, and in some scenes the difference is difficult to notice.
- **4K + DLSS Quality:** the RR cost is substantially less noticeable than 4K DLAA.

The 4K DLAA cost was tested extensively across repeated RR toggles, resolution changes, DLSS modes, and RT configurations. At this point it appears to be a real workload cost rather than an obvious mod-side recovery or state bug.

## Frame Generation

- **Off / 2x / 3x / 4x / 5x / 6x** fixed FG modes on supported hardware.
- **Dynamic MFG** with automatic monitor-refresh targeting or a manual 30–1000 FPS target.
- RTX 40-series safety policy: **Off + 2x only**.
- Live HDR/SDR switching without restarting Control.

### HDR/SDR recovery

When Windows changes HDR state while FG is active, Control FG commits DLSS-G off, frees the DLSS-G feature resources, waits for the new display domain to settle, requires fresh tagged frames, and then cleanly rearms FG. This also covers no-`ResizeBuffers` transitions and delayed duplicate domain notifications.

## HUD/UI handling

Control FG captures Control's full-resolution scene before the game draws its HUD/UI and handles the UI separately. Generated frames therefore do not treat objective text, health/energy elements, prompts, icons, or menus as moving world geometry.

## Overlay

Press **F10** to open the Control FG overlay. Settings are saved automatically in `%LOCALAPPDATA%\ControlFG\settings.ini`.

Dynamic Target FPS controls only appear while Dynamic FG is selected. In fixed FG modes the section is completely collapsed.

## Install

Copy `dxgi.dll` and the `ControlFGStreamline` folder beside `Control_DX12.exe`, launch Control in DX12 mode, and press **F10**.

See `INSTALL.md` and `TROUBLESHOOTING.md` for complete instructions.

## Future roadmap

The next major development target is **DLSS 5 integration**. RTX 20/30-series Frame Generation and Multi Frame Generation support is planned as a later compatibility research track.

## Support logs

If you need to report an issue, run `Collect-ControlFG-Compact-Logs.cmd` after reproducing it once and attach the resulting ZIP.

## Legal

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, AMD, Valve.
