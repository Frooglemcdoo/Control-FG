# R150 — Paired direct-light capture

## R149 result

The supplied R149 log prepared both exact GI variants and recorded their overrides. The user reports that the game became far too dark without correcting the shadow loss. The video shows severe darkening with the isolation mode selected. Removing baked diffuse-GI samples is not a usable fix. It does not establish exposure or all baked lighting as the cause. Use Default for normal rendering.

## What this build measures

R146 recorded only the accumulated diffuse/specular output after direct-light composition. R150 adds a read-only sample immediately before that same dispatch, at the same 4096 grid pixels, then samples again immediately after it. Each 128-byte pair includes pixel coordinates, selected light IDs, visibility and unclamped HDR diffuse/specular values. The decoder rejects inconsistent pair identities, invalid numbers or mismatched ABI lengths and reports signed after-minus-before deltas.

The delta is the net change of the whole observed direct-light pass, not an isolated spotlight or final display contrast. Point lights and other work in that dispatch may contribute. Later lighting/postprocessing and exact tracking of the same receiver across frames remain separate questions. Global averages compare changing sample coverage and cannot prove temporal loss.

## Test

1. Close the game and install beside Control_DX12.exe, preserving installed runtimes.
2. Load the affected area with your usual settings. In F10 select Default, close the overlay and stand before the doorway.
3. Press Shift+F9 once. Walk into the room and back slowly for about 25 seconds. Record video of the same walk. Keep focus on the game until the completion message; F9 cancels.
4. Package the resulting Shadow-R150 folder using Package-Lighting-Capture.cmd and send all ZIP parts with the video. No GI-off comparison is requested.

## Safety and validation

The paired samplers write only the private diagnostic buffer. Barriers order reads around native UAV writes; native descriptors and PSO are restored before the game dispatch. Pre/post samples must share the frame, command-list generation and heap. Both shader variants must prepare before capture is offered. Existing separate ray/composition queue fences and reset/export requirements remain. No CPU/GPU waits or render-thread file I/O are added. The existing six-sample timing is unchanged.

Windows WARP tests execute both samplers around a known lighting update across all 4096 pixels, verifying original/final HDR, IDs, visibility and positive/negative changes. Decoder tests retain old 64-byte captures and reject malformed pairs. These checks are not gameplay validation. Accepted rendering shaders, centered filtering, normal correction, authored clipping, No History default, GI filtering and exposure are preserved.
