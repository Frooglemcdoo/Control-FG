# GI32 — Live Jitter A/B + DGI Bounces 1–16

GI32 keeps the clean GI30 geometry baseline and makes the two remaining image-quality experiments live.

## Fixed baseline

- Direct RenoDX DLF shaders: **ON by default**.
- Reflection clamp: **60%**.
- Specular MV: cleared.
- Reflection MV: cleared.
- Specular hit distance: cleared.
- RR matrices: identity.
- Contact-shadow routing unchanged.

## Live controls

### Jitter mode
- **CONTROL** — Control's normal DLSS/SR jitter cadence.
- **RR 1024** — the historical native-RR 1024-frame jitter period.

### DGI bounces
- **NATIVE** — game-provided `g_uDGIPassCount`.
- Slider exposes **every integer from 1 through 16**.

Changing jitter, DGI bounces, or Direct DLF increments the GI32 quality generation and forces an RR history reset on the next evaluation.

## Test

Use the same low-light/shadow-boiling scene. Keep Direct DLF ON. Start with CONTROL + NATIVE, then raise DGI bounces one step at a time until the noise either disappears or stops improving. Note the lowest clean value and the performance cost. Then compare CONTROL versus RR 1024 at that same bounce count.

Expected markers:

- `RR_GI32_JITTER ... mode=control|rr_1024`
- `RR_GI32_DGI_BOUNCES ... native=... requested=... effective=... max=16`
- `RR_GI32_QUALITY_RESET ...`
- `RR_GI30_RENODX_BASELINE ...`
- `RR_GI29_DLF_MODE ... requested=1 enabled=1`
