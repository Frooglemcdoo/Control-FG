# PT1 - Inline RayQuery Reflection Proof

PT1 is the first custom ray-traced rendering pass, not another log-only probe.

It keeps the normal GI30/RenoDX RR baseline and leaves the PT1 pass OFF by default.

## F7 custom ray mode

Each F7 press advances:
1. HIT/MISS - ray hit = white, miss = black
2. HIT DISTANCE - hit distance as grayscale
3. INSTANCE ID - pseudo-color by committed DXR instance ID
4. OFF - native reflection output only

The custom pass runs immediately after Control's native reflectionRayGeneration DispatchRays call and writes into Control's reflection target. It uses DXR 1.1 inline RayQuery against the TLAS captured from Control's own top-level acceleration-structure build.

## F8 PT0 detail capture

F8 retains the 600-Present detailed PT0 census if extra evidence is needed. It is not required for the normal PT1 test.

## Test

Start with normal ray tracing and RR. Go to a scene with obvious RT reflections. Press F7 once and look for a stark black/white diagnostic in reflected lighting. Press again for hit-distance grayscale, again for instance-ID colors, and again for OFF.

Send the compact logs whether it renders or fail-closes.

Expected markers:
- PT1_READY
- PT1_EARLY_DXR_HOOK
- PT0_BUILD_AS
- PT1_INLINE_RAY_READY
- PT1_MODE
- PT1_CUSTOM_DISPATCH_OK

PT1_INLINE_RAY_SKIP is fail-closed evidence and leaves Control's native reflection pass intact.
