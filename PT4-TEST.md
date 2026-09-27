# PT4 — Full-Scene Path-Traced Lighting

PT4 turns the proven PT3 ray path into a view of the **actual game** instead of a synthetic gray beauty replacement.

## What BEAUTY does

- traces a camera primary ray through Control's live `g_rtScene`
- uses Control GBuffer1/GBuffer2 + MaterialDataPart1 for the visible primary surface
- traces one stochastic diffuse and one roughness-driven specular secondary ray per hit pixel
- converts those ray results into a bounded full-frame lighting multiplier
- copies Control's fully authored final frame to a private texture at the proven late-present boundary
- composites the PT lighting over that native frame, preserving textures, character shading, transparencies, volumetrics, particles, post processing and UI
- uses stable per-pixel stochastic directions in this build to avoid frame-to-frame glitter before temporal accumulation is added

PT OFF is still the true native Control RT/RR baseline.

## Important scope

This is the first **full-scene visual integration** build. It is not yet a claim that every Control light/material at arbitrary secondary hits has been reconstructed into a standalone multi-bounce renderer. Native authored surface color/direct-light/post information is deliberately retained while the path-traced visibility/indirect/specular field is applied across the whole frame.

## Test

1. Launch normally and establish the same camera view with PT OFF.
2. Open F10 and turn PT ON with **BEAUTY** selected.
3. The game should remain fully recognizable/color-correct instead of becoming the gray PT3 synthetic view.
4. Move through several bright and dark rooms and compare PT OFF/ON.
5. Check Jesse, glass/transparency, particles/volumetrics and HUD.
6. If anything freezes, becomes black, or the native frame fails to return, turn PT OFF and collect compact logs.

Expected markers:

- `source_revision=v2.1.1-pt4-full-scene`
- `PT3_DISPATCH_OK ... mode=beauty`
- `PT4_FULL_SCENE_READY`
- `PT4_NATIVE_SCENE_COPY_READY`
- `PT3_COMPOSITE_OK`
