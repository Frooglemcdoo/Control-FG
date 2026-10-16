M12R87 - terminal diffuse GI-volume input

PT now samples the authored GI volumes at its fourth (terminal) surface vertex,
weighted by that surface's diffuse material and path throughput. This replaces
part of the indirect tail that previously ended at zero. The first three vertices
retain traced transport. No native RT GI or reflection output is blended back in.

The cache is an approximation, not a complete physical light-source replacement.
It supplies only terminal diffuse lighting inside authored volumes. It does not
restore all missing static lights or supply terminal glossy lighting. Reflection
paths can benefit when they reach diffuse lit surfaces, but RR reflection distance
and motion guidance remain unresolved. R86 primary material composition remains.

Bindings come from the same-view native reflection pass. Absent cache inputs give
zero cache contribution, not a new switch to native lighting. Malformed or missing
volume data is bounded and skipped. Diagnostics report cache admission and positive
GPU samples. No extra rays, dispatches or per-pixel buffers. Cache sampling costs
GPU work at terminal hits; no FPS gain is claimed. PT-off/settings are unchanged.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Run Collect-PT-Primary-M12R87.cmd after normal gameplay.
