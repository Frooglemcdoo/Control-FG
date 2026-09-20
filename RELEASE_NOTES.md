# Control FG v2.0.0 — Release Notes

Control FG v2.0.0 is the Ray Reconstruction and renderer-stability update for Control DX12.

## New in v2.0.0

- **DLSS Ray Reconstruction** integrated into Control's native DX12 ray-tracing/denoising path.
- NVIDIA RR runtime **310.9.1** with **Preset F** default and live **E/F** selection.
- Finalized HDR/SDR Frame Generation recovery.
- Finalized HUD/UI handling for generated frames.
- Dynamic Target FPS UI now collapses completely outside Dynamic FG mode.

## Ray Reconstruction implementation

Ray Reconstruction is not applied as a screen-space effect after Control has already denoised its ray-traced image. Control FG integrates at the game's native RT denoising boundary.

When RR is enabled, the mod bypasses Control's native denoiser for the RR path and provides NVIDIA Ray Reconstruction with game-native renderer state including depth, motion vectors, camera and jitter state, ray-tracing resources, reset/discontinuity state, and current presentation/HDR context.

This keeps RR on the correct side of the renderer pipeline, avoids a double-denoise path, and allows it to coexist with Control's DLSS modes, ray tracing, HDR and Frame Generation.

## HDR / Frame Generation fixes

The final path performs a hard DLSS-G lifecycle reset across a display-domain change:

1. Frame Generation is committed off.
2. DLSS-G feature resources are explicitly freed.
3. Control is allowed to complete its HDR/SDR transition.
4. The mod waits for the new display domain to settle.
5. Fresh tagged frames are required before FG is rearmed.

The recovery also handles Windows HDR transitions where Control does **not** call `ResizeBuffers`, and suppresses delayed duplicate display-domain notifications so the same transition cannot trigger a second teardown.

This fixes the corrupted/generated-image state reproduced during repeated **Win + Alt + B** HDR switching in testing.

## HUD / UI Frame Generation fix

Control FG captures Control's actual full-resolution scene **before the game draws the HUD/UI**. Generated scene frames therefore do not have to interpret objective text, health/energy elements, prompts, icons or menus as moving world geometry.

The UI is handled separately and recomposited over the generated result. This substantially improves HUD stability during camera movement compared with feeding Frame Generation only the final composited image.

## Overlay fixes

- Lower-churn/double-buffered overlay drawing reduces flicker on title/menu screens and during screen recording.
- Expensive window/compositor operations are not repeated every timer tick.
- Dynamic Target FPS controls are shown **only** when Dynamic FG is selected.
- In fixed FG modes the entire Dynamic Target FPS section collapses, including Auto/Manual controls, target readout, slider and reserved blank space.

## Existing Frame Generation features retained

- Fixed DLSS Frame Generation / Multi Frame Generation modes from **2x through 6x** on supported hardware.
- **Dynamic Multi Frame Generation** with Auto monitor-refresh targeting or a Manual 30–1000 FPS output target.
- Dynamic pacing using NVIDIA Reflex and the PCL `SimulationStart` marker used by the validated runtime path.
- Persistent, Control-styled **F10 overlay**.
- RTX 40-series mode policy exposing **Off + 2x** while disabling unsupported higher MFG/Dynamic choices.

## ReShade / RenoDX compatibility

Control FG uses `dxgi.dll`. If ReShade also uses `dxgi.dll`, rename the ReShade DLL to `d3d12.dll` and leave Control FG as `dxgi.dll`.

## Verified target

**Control — Steam DX12 — Steam build 21225456**

DX11 is not supported. GOG/Epic builds and later Control patches are not claimed compatible until tested.
