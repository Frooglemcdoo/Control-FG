# R208: audited GI history-weight isolation

Based on the accepted R205 rendering baseline. R206 bypass removed reported
character ghosting but restored speckles. R207's per-frame history clear caused
flickering and moving glow. R208 includes neither experiment's rendering edits.

## Shader evidence

Recovered deferredlight_filtering_diffuse.obj from the user's loose-shader
archive Control-RR-Loose-Shaders-20260912-212234-687d93d6.zip. Its first DXBC is
5076 bytes, SHA256 d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15.
The checked-in base64 fixture is unmodified native bytecode, used for auditing.

DXBC dwords 123-197 load current color and apply a mip-based luminance clamp.
The CameraCut variable is b1 byte 24 (cb1[1].z). Dwords 204, 214 and 219 produce
(1-CameraCut)*0.7. Depth and motion confidence scale this factor at 325 and 373.
The final instruction at 403 blends clamped current color toward sampled history.
Thus zero history leaves the weight active and darkens current output; CameraCut=1
zeros the weight while retaining current-frame energy processing. The exact
instruction assertions are in tools/test-gi-temporal-audit.py.

## Runtime change

Only selected Native Blend GI frames request the override, latched at frame
start. The renderer's exact GI temporal bind call at +0x15691 and dispatch at
+0x156d3 are hooked using existing checked call-patch infrastructure. Shader
record identity must match the audited native SHA before activation. A bounded
single-entry cache checks descriptor identity on subsequent binds.

The existing verified thread-local CameraCut provider lease is forced to 1
across only that bind and dispatch, then restored before native spatial filtering.
An outer GI-filter finally restores again on exceptions. Missing dispatch,
unknown shader, arm/verification failure or restoration failure fails the RR
frame rather than claiming the isolation ran. Hook initialization rolls back
both new call sites if installation fails. Original filter arguments/pass count,
resource ownership and native history buffers remain unchanged.

No history clears, texture reallocation, per-frame RR resets or shader rewrite.
Projection rays, MAP GUARD OFF, restored reflection view and world albedo remain
as R205. Normal Native and RR-off rendering do not request the GI lease.

## Test

Use Native Blend, RR ON, SPEC Native, hit distance OFF, FG OFF and the accepted
projection-ray/MAP GUARD OFF defaults. Repeat the carpet movement, then inspect
the projector-room floor. Check ghosting, speckles, brightness, flicker and glow.
Send a short clip, screenshot and F10 SAVE LOG. No F9 suite is required.

The log should report gi_temporal_zero build=R208 applied=1 dispatches=1
restored=1 history_clear=0, with the native spatial pass count. This proves CPU
recording/verification, not GPU image quality. User testing remains necessary.

Validation: exact native shader/operand audit, semantic weighting fixtures,
production-hook mocks for normal/native paths, shader mismatch, provider failure,
bind/dispatch exceptions and restoration before spatial work; baseline checks
and the Windows build's existing WARP/integration suite.
