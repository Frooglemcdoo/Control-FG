M12R79 - fresh current-frame PT input for RR

R78 capture: no reconstruction setup failures or consumer rejections were logged.
Visible noise and motion smearing remain. This build corrects a separate input
contract: the custom PT temporal/spatial denoiser previously ran before RR.

For an engine frame confirmed selected for full RR after native feature setup,
PT now supplies current diffuse + specular radiance without custom temporal
accumulation, history clipping or spatial filtering. RR retains its own history.
The route uses an atomic exact engine-frame identity, not the requested UI mode
or a previous successful RR evaluation. The custom filter remains for non-RR
frames; history produced by the fresh-input route cannot seed that filter.
Coverage, producer admission, unsupported-material fallback and settings remain.
No extra rays. Two PT spatial dispatches are omitted on selected RR frames.

PT_PRIMARY_RECONSTRUCTION logs fresh_rr_input=1 and history=0 for this route,
plus engine_frame for correlation with RR_FRAME_MODE and RR_NATIVE_EVALUATED.
Fresh input is not a claim of successful later RR evaluation: failures after
frame selection still follow the existing terminal/fail-closed coordinator.

WARP tests check exact current diffuse+specular, unchanged checker samples,
lights-off without stale accumulation, RR-off history isolation, admission and
coverage. Existing legacy/split reconstruction tests remain. These tests cannot
establish in-game RR quality or performance.

Remaining: valid PT specular-hit-distance integration, native moving-object
motion validation, missing light sources and unsupported materials. The current
PT continuation distance mixes lobes and is NOT passed off as a specular guide.
This is a targeted contract correction, not a claim all flicker is resolved.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Collect with Collect-PT-Primary-M12R79.cmd.
