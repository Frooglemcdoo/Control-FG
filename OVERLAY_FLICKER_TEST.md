# Control FG v1.0.0 - Overlay Flicker Stability Test R1

This test is based on the current v1.0.0 production source. The frozen v0.8.26 frame-generation, HDR10, Reflex, PCL, resource-tagging, and presentation paths are intentionally unchanged.

## Overlay-only changes

- Paint the complete Win32 panel into an off-screen compatible bitmap, then publish it with one `BitBlt`.
- Keep the existing 50 ms F10/input timer, but stop doing compositor-heavy work on every timer tick.
- Cache the Control game HWND instead of enumerating process windows continuously.
- Retry game-window discovery no more than once per second while unresolved.
- Poll overlay placement every 250 ms and call `SetWindowPos` only when the computed rectangle actually changes.
- Call `ShowWindow` only when overlay visibility changes.
- Refresh passive status fields at 4 Hz; mouse/button/slider interactions still repaint immediately.
- Add `FG_OVERLAY_CADENCE` telemetry every ~5 seconds while the overlay is visible.

## Build

1. Extract the entire source ZIP.
2. Close Control.
3. Run `Build.cmd`.
4. Confirm `Build.log` contains `Applied Control FG overlay stability test R1.` or `Overlay stability test R1 is already applied.`
5. Install using the normal v1.0.0 test workflow.

## Test sequence

Open the overlay with F10 and compare each case for about 20-30 seconds:

1. Start/title screen, no recording.
2. Start/title screen while screen recording.
3. In-game pause/settings menu, no recording.
4. In-game pause/settings menu while screen recording.
5. Normal gameplay while recording.
6. FG Off, then one normal FG mode, to confirm the visual behavior remains independent of FG.

The key question is whether the overlay remains visually stable during the menu/title-screen and recording cases that previously showed the strongest flicker.

After the test, quit Control and run `Collect-ControlFG-Logs.cmd`. The runtime log should contain:

- `FG_OVERLAY_STABILITY_TEST revision=R1`
- `FG_OVERLAY_VISIBILITY_TRANSITION`
- `FG_OVERLAY_CADENCE`

A healthy idle overlay should show roughly 4 paints/sec, zero placement updates unless Control actually moves/resizes, and no repeated show/hide transitions while it remains visible.
