# PT1 R2 — TLAS Descriptor + Post-Deferred Reflection Proof

This fixes the two failures proven by the first PT1 runtime log.

- Control's primary shader-visible CBV/SRV/UAV heap contains **506,144 descriptors**. PT1 R1 only retained descriptor metadata for heaps <=262,144, so `g_rtScene` could never be recovered and every F7 mode fail-closed with `reason=tlas_unavailable`.
- R1 attempted its debug write immediately after `reflectionRayGeneration`. Control performs deferred reflection shading after that ray pass, so a successful write could be overwritten before composition.

R2 retains metadata for the large Control heap, captures the RTAS SRV GPU address as `PT1_TLAS_DESCRIPTOR`, and executes the diagnostic at the **next native RT pipeline setup**, after Control has finished deferred reflection shading.

## F7
OFF -> HIT/MISS -> HIT DISTANCE -> INSTANCE ID -> OFF

## Expected proof
After one F7 press:
- `PT1_TLAS_DESCRIPTOR ... gpuva=...`
- `PT1_DEFERRED_BOUNDARY ... tlas=0x...`
- `PT1_INLINE_RAY_READY ready=1`
- `PT1_CUSTOM_DISPATCH_OK ... boundary=post_deferred_reflection`

The first mode should be visually unmistakable in RT reflections: white ray hits and black misses.

F8 remains the PT0 detailed-capture hotkey and is not required unless requested.
