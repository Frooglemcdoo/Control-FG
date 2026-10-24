# R143 — room-transition shadow signal comparison

The user reports shadows becoming difficult to see while entering new rooms,
also in the Native comparison. Exposure adaptation itself is acceptable and
must remain enabled. The video does not establish that exposure causes the
shadow loss. This build adds diagnostics, not a claimed rendering fix.

Default (Fixed) is byte-for-byte the R142 shader path: R134 authored clipping,
R139 surface-normal selection, current-frame spotlight visibility and centered
spatial filtering. Point-light history, GI filtering and saved preferences remain
unchanged. No new GPU resources, readbacks, waits or shader binaries are added.

## Live test

Close Control to install. Retain the existing NVIDIA runtimes. Use the same game
settings as the problem video, except turn FG and glass enhancements off. Keep
RT Contact Shadows on and game RT samples at 1 so the corrected path can run.

Record one video, repeating the same doorway/stair movement for each of these:

1. **Default (Fixed)**: normal appearance.
2. **RR Off**: normal appearance with only RR bypassed. This is the fair full-color
   reference for signal modes, which also bypass RR.
3. **Shadow Signal**: selected spotlight visibility after the accepted spatial
   filter. Dark means shadowed; white means unoccluded; magenta means no selected
   spotlight. This is not a display of every shadow/light in the scene.
4. **Raw Signal**: same selection and authored rays, but without spotlight spatial
   filtering. It also uses current-frame visibility. Compare the shape and extent
   of the shadow with Shadow Signal.
5. **Light IDs**: false colors identify the selected spotlight. Magenta again
   means no selected spotlight. Colors are diagnostic labels, not brightness.

Use F10 to select a mode, then close the panel before moving. Allow the mode to
settle before each pass. Return to Default (Fixed) afterwards and click Save Log;
send that log with the video. No restart is needed between modes.

The old History On/Guard buttons move out of the nine-button panel but remain
in F9. F9 now includes 28 unique comparisons / up to 84 PNGs, including all three
new signal modes; the prior unclipped signal modes retain explicit legacy names.
F9 is stationary and cannot replace the movement video. GPU hit capture remains
in Both and therefore is not proof of the new default's clipped ray result.

## What this can and cannot establish

The signal shaders sample t15.x visibility and t14.x selected spotlight identity
before native direct material/light accumulation. They replace composition's
diffuse output with the diagnostic value and zero its specular output. The
producer still applies the corrected surface-normal selection. Unlike the old
visibility views, these modes retain authored clipping and No History.

These views still pass through downstream game processing. Exposure remains
enabled and can respond differently to a diagnostic image. They are **not raw
HDR captures or exposure-independent measurements**; displayed intensity alone
cannot prove a tone-mapping defect. Compare boundaries, shadow shape, and ID
changes, not brightness between modes. Magenta does not mean all lighting has
been culled: another spotlight, point light or baked contribution can exist.

If the shadow shape vanishes in both signal views, investigate selected-light
coverage/identity and ray visibility. If Raw retains it while filtered loses it,
investigate spatial filtering. If the signal remains but full-color loses its
contrast, investigate contribution mixing and postprocessing next. These are
branches for further investigation, not diagnoses established by this build.

Validation: mode policy and readiness, required producer/composition evidence,
RR bypass, authored-ray routing, distinct composition PSOs, and current-frame
history versus filtered/raw spatial routes. Windows/WARP validates routing and
existing shader execution; it cannot validate the reported in-game transition.
