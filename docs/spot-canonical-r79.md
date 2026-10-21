# R79: one R71 ray-generation binary for both native shader variants

R78 still produced the blob. A fresh R71 run cannot activate because the game now
creates the configurable native library d5efe2c2..., while the earlier working R71
run used 3c6bee7e.... R78 only aligned the spotlight sampling algorithm; it still
used a different complete ray-generation shader for the configurable variant.

## Verified engine selection

In the hash-verified renderer, the option `rt:Sample Count Shadow` is registered
at object RVA 0x913be0 (default 1). Its effective integer at 0x913c90 is read at
0x23f855 and submitted as `g_uRTContactShadowRayCount`. At 0x23f8b5 the renderer
compares that value with 1, then uses the result to choose the rt_spotDeferredShading
variant on first creation. The resulting shader pointer is cached at 0x128da40.
The captured configurable shader loops over that constant. The single variant
has no dependency on the deferred-light constant buffer.

The specific user-facing menu/configuration change that changed this value has
not been established. This is not evidence that native multi-sample shadows are
inherently broken.

## Implementation

Both recognized native-library hashes now receive the exact R71 replacement
binary, f0127f6267359eccf61fda126ecc9ff733d0b9090f6e9d16a88f65840e7d3ab8.
The separate configurable replacement and its packaging dependency are removed.
An old rt_spot_multi.dxil file is ignored. Unknown native libraries still retain
native rendering. Native root signatures, hit/miss shader tables and alpha-tested
hit handlers are retained.

Captured binding tables show that every resource required by the R71 library has
the same register/space, type and count in the configurable native library. The
extra native cb1 binding is not used by the replacement. Payload and structured
light/hit layouts match.

This also restores R71's **single-sample point-contact-shadow branch** while RT
SPOTLIGHT is active. R78 retained the configurable point loop. Spotlight shadows
still use four symmetric rays. The engine's saved sample count is never written;
NATIVE SHADOWS returns to its native configured shader. Logs record the effective
native count and `replacement=R71_exact` to make this distinction explicit.

R71's temporal/spatial filters, replacement light composition and native caster
coverage remain. No regular shadow-map fallback is introduced in RT SPOTLIGHT.

## In-game check

Close Control to install the complete package. Enable RT Contact Shadows, enter
the affected room, then select F10 > RT SPOTLIGHT. Compare against NATIVE SHADOWS
live. Check the blob first, then noise, Jesse's shadow and pop-in. Save one R79 log.
No restart is needed between overlay modes.

This removes the alternate replacement as a variable; it is not yet a confirmed
blob fix. If the blob remains, the configured variant alone cannot explain it.
CPU submission logs and WARP compute checks do not prove the final DXR image.
