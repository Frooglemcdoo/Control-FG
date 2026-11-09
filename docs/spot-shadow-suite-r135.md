# R135: shadow comparisons and repeatable F9 evidence

R134 AUTHORED CLIP was confirmed by the tester to restore missing lighting in all affected rooms tested. Preserve that signed shader byte-for-byte. The next reported issue is streaked/overlapping moving character shadows. Its cause is not yet proven. All four existing spotlight rays target the same emitter center; spreading four samples is not the cause in this baseline.

## Comparisons

- NATIVE: no diagnostic lighting overrides. Retains the user's saved graphics/RR settings.
- BOTH: previous centered spotlight plus GI policy, without the authored near plane.
- DEFAULT (CLIP): R134's confirmed authored boundary. Automatically selected once its shaders are ready, unless the user already selected a mode.
- NO HISTORY: same authored rays and spatial softness, but current-frame spotlight visibility replaces temporal history. Point-light history and the RR choice remain unchanged. This is a candidate for motion trails, not a confirmed fix; compare while moving Jesse.
- CLIP RAW: same authored rays with spotlight temporal and spatial filters bypassed. Hard edges are expected. This isolates filtering without losing the boundary fix.

The top row is NATIVE / BOTH / DEFAULT (CLIP). Hit X/Y/Z and light-ID comparisons remain available. No new rays, shader compilation, or per-frame capture allocations are added to normal play.

## F9

With the game in the foreground at a problem location, press F9 once and keep the camera still. The panel closes; the suite saves your selected mode, runs 13 comparisons, and restores that mode. Press F9 again to cancel. Changing focus cancels rather than photographing another application. Each mode waits for elapsed time, rendered presents and (where available) its binding observation. Three PNGs are collected per mode. Expect roughly 40–60 seconds; each step has a 20-second timeout.

The suite includes native, Both, default clip, clip without history, clip raw, GI-only, selected-light-open, raw/filtered visibility, light IDs and hit X/Y/Z. Diagnostic colors are unclipped reference views and may bypass RR; the manifest labels them accordingly. Availability failures/timeouts are recorded, never called successful captures.

During BOTH it requests the validated 64x64 native-input/GPU-hit capture: ray endpoints, barycentrics, primitive/instance/geometry IDs, per-light cone/emitter/direction/radius and view-to-world matrix, table validation and fence completion. Export is asynchronous. GPU buffers are reused only after completion, export and a reset of the original native recording; each reuse gets an increasing fence value. Failed jobs are quarantined. F9 no longer accidentally triggers the separate RR-material pair capture (Ctrl+Alt+F9 remains available in this diagnostic build).

Output: `%LOCALAPPDATA%\Control-FG\Diagnostics\Shadow-R135-...` (temporary-folder fallback if needed). A completion dialog shows the exact folder. Zip that folder and send it. It contains `manifest.txt`, `lighting.log`, and PNGs. Each run's log is sliced at its own start so previous GPU captures cannot be mistaken for this location. The regular session log is retained separately.

PNG capture reads the displayed game client on the UI thread, not a swapchain resource, and introduces no explicit GPU fence wait. These are display-referred comparisons, not raw HDR/color-calibrated buffers. Exclusive fullscreen/driver composition can yield black images; check the first set. This is not a PIX capture and does not collect every historical experimental shader or full RR input texture dump. Keep FG off for diagnostic runs. Shader and renderer observations are in the log; camera position is not programmatically locked and temporal trails still benefit from a short motion video.

## Validation and scope

Local mode/settling tests pass. The R134 authored shader's hash and source-reversal tests pass. Windows CI must compile the complete DLL and exercise mode routing and shader creation. Visual improvement from NO HISTORY requires tester confirmation; if it changes nothing, use the F9 raw/filtered/ID evidence before altering softness or light selection.
