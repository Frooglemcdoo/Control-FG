# R158 — isolate the incorrect spotlight shadow

R157 successfully captured two consecutive RR-off frame pairs before and inside the room. The broad boundary is present in accumulated diffuse lighting before final tone mapping. The user confirmed from the light position that the broad diagonal shadow should not exist. This does not yet identify the responsible light or blocker.

RT SPOT OPEN is a matched full-color comparison against RR OFF: only the selected spotlight visibility operands in diffuse and specular composition become one. It preserves geometric surface normals, light selection, authored clipping, no-history policy, GI filtering, other spotlights and point-light maps. The old RoomOpen used an older composition baseline and was not a matched comparison. Default rendering remains unchanged.

## Test

1. Start a fresh session with usual DLSS Quality/DLAA and RT settings. Stand where the incorrect shadow is visible.
2. Select RR OFF. Press Shift+F9 once and hold still through the five-second settling period. Check Saved pairs: 1/2.
3. Keep the same camera position. Select RT SPOT OPEN. Press Shift+F9 once, hold still, and check Saved pairs: 2/2.
4. Click PACKAGE LIGHT and send every ZIP plus a short comparison video. The mode is recorded in each frame. Pair labels before-room/inside-room refer to acquisition order in this test.

If the incorrect shadow disappears, selected RT spotlight visibility is implicated. If it remains, investigate the remaining light/map/selection/composition paths. Disappearance of legitimate selected-light shadows is expected in this diagnostic and is not a proposed fix. This is not an all-shadows-off mode.

The new optional shader has independent readiness; failure cannot disable Default. No GPU readbacks or new render-thread I/O are added. Shift+F9 retains the existing capture path and plain F9 retains the stationary suite. GI + RR OFF remains in the suite but its overlay shortcut is replaced by RT SPOT OPEN.
