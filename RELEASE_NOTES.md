# Control FG v2.0.0 — Release Notes

v2.0.0 is the Ray Reconstruction and renderer-stability update for Control DX12.

## New in v2.0.0

- **DLSS Ray Reconstruction** integrated into Control's native DX12 ray-tracing/denoising path.
- NVIDIA RR runtime **310.9.1** with **Preset F** default and live **E/F** selection.
- Finalized HDR/SDR Frame Generation recovery.
- Finalized HUD/UI handling for generated frames.
- Dynamic Target FPS UI now collapses completely outside Dynamic FG mode.

## Ray Reconstruction implementation

Ray Reconstruction runs at Control's native RT denoising boundary. When RR is enabled, Control FG bypasses Control's native denoiser for the RR path and provides RR with game-native depth, motion vectors, camera/jitter state, ray-tracing resources, reset state, and HDR/presentation context.

This avoids a double-denoise path and allows RR to coexist with Control's DLSS modes, ray tracing, HDR, and Frame Generation.

## Performance

Extensive testing shows the largest RR cost at native 4K with DLAA:

- **4K + DLAA:** roughly **20–30%** lower performance with RR enabled.
- **1440p / 1080p:** typically around **2–10%**, often difficult to notice depending on the scene.
- **4K + DLSS Quality:** the RR cost is much less noticeable than 4K DLAA.

The 4K DLAA hit was reproduced across repeated RR toggles, resolution changes, DLSS modes, and RT configurations and currently appears to be a real workload cost rather than an obvious mod-side bug.

## HDR / Frame Generation fixes

Across an HDR/SDR transition, Control FG now:

1. commits Frame Generation off;
2. frees DLSS-G feature resources;
3. lets Control/Windows complete the display-domain transition;
4. waits for the new domain to settle;
5. requires fresh tagged frames before rearming FG.

This also handles transitions where Control does not call `ResizeBuffers` and prevents delayed duplicate HDR notifications from triggering a second teardown for the same transition.

## HUD / UI Frame Generation fix

Control FG captures Control's full-resolution scene before the HUD/UI is drawn and handles the UI separately. This substantially improves HUD stability during camera movement and prevents UI elements from being interpreted as moving world geometry by Frame Generation.

## Overlay fixes

- Lower-churn/double-buffered drawing reduces overlay flicker.
- Dynamic Target FPS controls appear only while Dynamic FG is selected.
- In fixed FG modes the entire Dynamic Target FPS section collapses with no reserved blank space.

## Existing FG features retained

- Fixed **2x through 6x** FG/MFG modes on supported hardware.
- **Dynamic MFG** with Auto monitor-refresh targeting or Manual 30–1000 FPS targeting.
- RTX 40-series policy exposing **Off + 2x**.
- Persistent F10 overlay and settings.

## Next

The next major development target is **DLSS 5 integration**. RTX 20/30-series FG/MFG compatibility research is planned after that.

## Verified target

**Control — Steam DX12 — Steam build 21225456**
