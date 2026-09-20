# Control FG v1.0.0

First public release of Control FG for **Control (Steam DX12)**.

## Highlights

- NVIDIA DLSS Frame Generation / Multi Frame Generation: **2x, 3x, 4x, 5x and 6x** on supported hardware.
- **Native Dynamic MFG** with automatic monitor-refresh targeting or a manual 30–1000 FPS output target.
- **DLSS Ray Reconstruction** integrated directly into Control's native DX12 ray-tracing path.
- RR runtime **310.9.1**, with **Preset F** as the default and live **E/F** switching from the overlay.
- **Live SDR/HDR transitions** with a hard DLSS-G resource reset/rearm path so FG does not carry stale presentation state across Windows HDR changes.
- **Game-aware HUD/UI handling** using Control's real pre-UI scene plus separate UI recomposition for stable generated frames.
- Control-styled **F10 overlay** with persistent settings and runtime status.
- RTX 40-series safety policy: **Off + 2x only**.

## Ray Reconstruction

Ray Reconstruction is implemented at Control's native ray-tracing/denoising boundary rather than as a final-image post-process. When RR is enabled, Control FG bypasses the game's native denoiser for that path and supplies RR with renderer data captured from Control itself, including depth, motion vectors, camera/jitter state, ray-tracing resources, reset state, and the current HDR/presentation context.

This avoids a double-denoise path and lets RR reconstruct the ray-traced image from the same game-native inputs used by the rest of the integration. RR is designed to operate alongside Control's DLSS modes, ray tracing, HDR, and Frame Generation.

## Important fixes in this release

- **HDR/SDR FG recovery:** Windows HDR changes no longer leave DLSS-G stuck in the previous presentation state. The mod now turns FG off, frees DLSS-G resources, waits for the new display domain to settle, requires fresh tagged frames, then cleanly rearms FG.
- **No-resize HDR recovery:** Handles Windows HDR transitions where Control does not issue a new `ResizeBuffers` call.
- **Duplicate-transition suppression:** Prevents delayed HDR detection from starting a second teardown for the same display transition.
- **HUD/UI stability:** Generated frames use a real HUD-less scene capture and separate UI recomposition, reducing HUD ghosting/distortion during camera motion.
- **Overlay cleanup:** the Dynamic Target FPS controls are only shown while Dynamic FG is selected; the section collapses completely in fixed FG modes.
- **Overlay stability:** lower-churn/double-buffered overlay drawing reduces menu/title-screen flicker and recording-related flicker.

## Compatibility

Verified target: **Control on Steam, DX12, Steam build 21225456**.

Other storefront builds and later game patches are not claimed compatible until tested. DX11 is not supported.

## Installation

Download `Control-FG-v1.0.0.zip`, then copy `dxgi.dll` and the `ControlFGStreamline` folder next to `Control_DX12.exe`. Launch Control in DX12 and press **F10**.

See `INSTALL.md` and `TROUBLESHOOTING.md` for complete instructions.

## ReShade / RenoDX

If ReShade is installed as `dxgi.dll`, rename the ReShade DLL to `d3d12.dll` and keep Control FG as `dxgi.dll`. This allows both loaders to coexist. The same approach is expected to work with RenoDX configurations that use the same injection method.

## Notes

Control FG is an unofficial fan-made mod and is not affiliated with or endorsed by Remedy Entertainment, 505 Games, NVIDIA, AMD, Valve, ReShade, RenoDX, or any other third party.
