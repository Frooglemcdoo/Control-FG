# Control FG v2.0.0

Major update for **Control (Steam DX12)** adding DLSS Ray Reconstruction plus the final HDR, HUD/UI, and overlay fixes from the v2.0.0 development cycle.

## Highlights

- **DLSS Ray Reconstruction** integrated directly into Control's native DX12 ray-tracing/denoising path.
- NVIDIA RR runtime **310.9.1**, with **Preset F** as the default and live **E/F** switching.
- NVIDIA DLSS Frame Generation / Multi Frame Generation: **2x through 6x** on supported hardware.
- **Native Dynamic MFG** with automatic monitor-refresh targeting or a manual 30–1000 FPS target.
- **HDR/SDR FG recovery** with explicit DLSS-G resource teardown and clean rearm.
- **Game-aware HUD/UI handling** using Control's real pre-UI scene and separate UI recomposition.
- Dynamic Target FPS controls now appear only while Dynamic FG is selected and the entire section collapses in fixed modes.

## Ray Reconstruction

Ray Reconstruction runs at Control's native RT denoising boundary rather than as a final-image post-process. When RR is enabled, Control FG bypasses Control's native denoiser for that path and supplies RR with game-native depth, motion vectors, camera/jitter state, ray-tracing resources, reset state, and HDR/presentation context.

This avoids a double-denoise path and lets RR operate alongside Control's DLSS modes, ray tracing, HDR, and Frame Generation.

## Performance

Extensive testing shows the largest RR cost at **4K + DLAA**:

- **4K + DLAA:** roughly **20–30%** lower performance with RR enabled.
- **1440p / 1080p:** typically around **2–10%**, and in some scenes the difference is difficult to notice.
- **4K + DLSS Quality:** the RR cost is substantially less noticeable than 4K DLAA.

The 4K DLAA hit was reproduced across repeated RR toggles, resolution changes, DLSS modes, and RT configurations and appears to be a real workload cost rather than an obvious mod-side bug.

## Important fixes

- **HDR/SDR FG recovery:** FG is committed off, DLSS-G resources are freed, the display-domain transition is allowed to settle, fresh tagged frames are required, and FG is then cleanly rearmed.
- **No-`ResizeBuffers` HDR recovery:** covers Windows HDR transitions where Control does not rebuild the swapchain.
- **Duplicate-transition suppression:** prevents delayed HDR detection from starting a second teardown for the same transition.
- **HUD/UI stability:** generated frames use Control's true pre-UI scene and separate UI recomposition.
- **Overlay cleanup:** Dynamic Target FPS controls are hidden and the entire section collapses unless Dynamic FG is selected.
- **Overlay stability:** lower-churn/double-buffered drawing reduces menu/title-screen flicker.

## Compatibility

Verified target: **Control on Steam, DX12, Steam build 21225456**.

DX11 is not supported. Other storefront builds and later game patches are not claimed compatible until tested.

## Installation

Download `Control-FG-v2.0.0.zip`, copy `dxgi.dll` and the `ControlFGStreamline` folder beside `Control_DX12.exe`, launch Control in DX12 mode, and press **F10**.

See `INSTALL.md` and `TROUBLESHOOTING.md` for complete instructions.

## Next

The next major development target is **DLSS 5 integration**. RTX 20/30-series FG/MFG compatibility research is planned after that.

## Notes

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, AMD, or Valve.
