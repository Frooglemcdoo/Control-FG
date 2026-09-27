# GI28 — RenoDX Signal Parity A/B

GI28 keeps the existing specular clamp implementation and **does not raise its default above 60%**.

The RR settings page adds two session-only A/B controls:

- **Diffuse current-frame clamp**
  - **RENODX** (default): Control's native diffuse temporal pre-accumulator is allowed to run with `g_uCameraCut=1` and `passCount=0`. This preserves the native current-frame firefly/energy clamp while disabling temporal history and spatial passes.
  - **OFF**: public v2.1.1/GI27 behavior; the diffuse filter boundary is bypassed for full RR.

- **Contact-shadow path**
  - **CURRENT MOD** (default): existing full-RR policy, temporal off and spatial size zero.
  - **RENODX**: leave Control's original contact-shadow filter arguments untouched.

The existing GI27 explicit specular-MV toggle remains available. The existing reflection-clamp slider remains unchanged; use 60% for the parity comparison.

Expected runtime markers:
- `RR_GI28_DIFFUSE_UI`
- `RR_GI28_DIFFUSE_CLAMP ... mode=renodx ... clamp=native_1_0 temporal_history=off spatial_passes=0`
- `RR_GI28_CONTACT_UI`
- `RR_GI28_CONTACT_SHADOW ... mode=current_mod` or `mode=renodx_native`

Recommended four-way comparison at the same camera/animation:
1. Diffuse OFF + Contact CURRENT MOD
2. Diffuse RENODX + Contact CURRENT MOD
3. Diffuse OFF + Contact RENODX
4. Diffuse RENODX + Contact RENODX
