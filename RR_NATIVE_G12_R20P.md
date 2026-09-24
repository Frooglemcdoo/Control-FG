# G12-r20p — explicit RR teardown + skin/SSS path + visible AA sharpness

r20p is a focused follow-up to r20n/r20o based on live image-quality testing.
It preserves the frozen production FG/HDR/Dynamic core and Preset F.

## 1. Explicit RR off transition

The previous option-isolation work kept Control's registered hidden RR option at
its native OFF baseline, but the Streamline RR viewport options were only
cleared in our local state. NVIDIA's Streamline 2.14.1 RR contract requires
setting `DLSSDOptions.mode = eOff` when RR is disabled.

r20p now sends that explicit eOff options update before clearing the RR option
handshake. Failure is retried on the next AA frame and is logged as
`SL_RR_OPTIONS_DISABLED`. AA/render-extent changes still rebuild the RR options
handshake.

This specifically targets the observed sequence:

`DLSS Quality + RR ON -> RR OFF -> DLAA -> half-RR-looking image`

## 2. Sharpness control is visible and scoped correctly

The F10 panel is now a new taller r20p layout with a visible
`AA SHARPNESS (SR / DLAA)` section. The control has AUTO and a 0.00-1.00 slider.
It is persisted in `%LOCALAPPDATA%\ControlFG\settings.ini`.

The override is applied to Control's existing native DLSS/DLAA wrapper only.
AUTO preserves Control's original negative-sentinel/default behavior.

DLSS-RR itself ignores the normal DLSS `sharpness` option in Streamline 2.14.1,
so r20p does not pretend this slider changes RR. While RR is requested the
control is disabled and the overlay explains that a real RR output-tuning path
will need a post-RR stage.

Expected markers:

- `AA_OVERLAY_SHARPNESS`
- `AA_SHARPNESS_OVERRIDE`
- `SL_RR_OPTIONS_DISABLED`
- `SL_RR_OPTIONS_REBUILD`

## 3. Deeper skin / SSS investigation

r20p expands the read-only skin diagnostic rather than applying a global
roughness/specular hack.

On both native SR and RR evaluation branches it samples the NGX parameter object
before and after evaluation for:

- `DLSSD.ScreenSpaceSubsurfaceScatteringGuide`
- `DLSSD.ColorBeforeScreenSpaceSubsurfaceScattering`
- `DLSSD.ColorAfterScreenSpaceSubsurfaceScattering`
- `GBuffer.Subsurface`
- `DLSSD.ResponsivityMask`

Any surfaced resources are descriptor-checked against the current input
resolution. A direct SSS guide is considered shape-compatible only when it is a
single-sample, one-mip `R16_FLOAT` texture at input resolution.

The same frame is correlated with the audited native opaque replay families.
Character batches now publish count, instance count, and a session-local
fingerprint through `RR_SKIN_CHARACTER_BATCHES`.

`RR_SKIN_PATH` chooses the next evidence-based route:

1. use a native SSS guide if Control exposes one;
2. derive NVIDIA's true SSS guide from native before/after SSS surfaces if both
   are exposed;
3. otherwise build a Character-only mask for a later RR-bypass/composite or
   responsivity experiment.

r20p does **not** synthesize a fake SSS guide from a binary character mask and
it does not yet composite native skin over RR.

## Test

1. Extract the ZIP into a fresh folder. Do not run it from inside the ZIP.
2. Run `Build.cmd`, then `Install.cmd`.
3. Open F10. Confirm the panel identifies r20p and the AA SHARPNESS section is
   visible under Ray Reconstruction.
4. With RR OFF, move the sharpness slider and verify DLSS/DLAA changes. Click
   AUTO to restore Control's native behavior.
5. Set DLSS Quality, enable RR, then disable RR. Switch to DLAA. Verify whether
   the previous half-state is gone.
6. Enable RR and reproduce the plastic-skin scene for 15-30 seconds with a
   character clearly visible.
7. Quit and run `Collect-ControlFG-Compact-Logs.cmd`.
8. Send the compact log ZIP plus one screenshot of the plastic-skin case.

The critical skin markers are `RR_SKIN_INPUTS`, `RR_SKIN_RESOURCE`,
`RR_SKIN_CHARACTER_BATCHES`, and `RR_SKIN_PATH`.
