# R91: isolate the missing RT shadow

R90 map coverage hid the desired RT appearance and can restore map noise. R91
removes that merge and restores the exact R89 ray and composition assets.

The F10 panel now offers NATIVE SHADOWS, RT FILTERED, and RT RAW. FILTERED is
R89, including its mean blocker softness change. RAW bypasses the spotlight
temporal and spatial filters while retaining its rays, light selection, alpha
handling and material lighting. Native point filtering is retained. No map
fallback or all-spotlight runtime is active.

This is a coverage isolation build, not a claimed coverage fix. R77 had raw
isolation controls, but the available feedback did not establish a raw-versus-
filtered result at this location; do not treat that earlier attempt as proof.

Close Control to install all files beside Control_DX12.exe. Keep RT Contact
Shadows enabled. Turn RR and FG OFF so final reconstruction and generated frames
do not obscure the comparison. Stand still at pillar 14. Compare RT FILTERED,
RT RAW, and NATIVE SHADOWS without moving the camera; wait a few seconds after
each switch. Send screenshots and one Lighting-R91 log. RAW will be grainy: look
for whether the rail and missing character silhouette exist at all. Return to
RT FILTERED after the test and restore your normal RR/FG settings.

If coverage exists in RAW but disappears in FILTERED, investigate the filter
inputs, history or rejection. If it is absent in both, investigate rays, caster
coverage, light selection and composition. RAW is rendered lighting, not a
direct hit-buffer dump, so this does not uniquely identify missing geometry.
