# R154 — native lighting capture, RR disabled

The user explicitly confirms the symptom in the base native game without RR. R151–R153 focused on RR boundaries and were not the right baseline for isolating this native lighting problem. R154 captures the native SR/DLAA path with RR bypassed and no RR guide requirement. Normal rendering and exposure are not changed by capture.

## R143 archive review

Reviewed Shadow-R143-20261024-140449-532-0.zip: 84 screenshots, manifest and log; 28 completed modes, zero failures. Native RR Off and Default RR Off have rr=0 in the manifest and weak/absent diagonal floor-door contrast. Corrected Shadow Signal and Raw Shadow Signal both retain the diagonal boundary across movement. This is actual selected-spot visibility evidence, stronger than light-ID or hit-position colors alone. Those signal views bypass the ordinary light/material accumulation, but still pass through later processing. Their high contrast does not prove ordinary shaded values are immune to exposure/tone mapping.

Selected Light Off leaves a pronounced floor boundary. Its shader omits the selected spot's final diffuse/specular additions while retaining nonselected accumulation and point-light work. The omitted light varies by pixel, so the boundary can also expose a light-selection seam: do not label it a restored physically correct shadow. Camera positions change during the suite; images are not registered identical-surface measurements. The GI-only name denotes the mod's GI-filter-only comparison, not an image containing exclusively indirect light.

Together, the evidence prioritizes native light accumulation and its application over another RR fix or shadow-geometry replacement. Exposure as sole cause remains unproven.

## New capture

Select NATIVE RR OFF, then CAPTURE LIGHT. Resume gameplay if the pause menu is open and hold still until Saved pairs: 1/2. Move to where the shadow becomes faint, repeat, wait for 2/2, then PACKAGE LIGHT. A nonblocking three-second settling gate restarts on frame gaps, resets or failed evaluations. Package ZIPs stay below 200 MB. No capture hotkey or beep is required. Missing resources produce diagnostics, not substituted data.

This focused path requires native DLSS SR/DLAA to provide the audited evaluation boundary; retain the existing DLSS Quality setting. It does not yet support DLSS completely disabled. RR is bypassed by NATIVE RR OFF regardless of saved preference. The diagnostic build invokes this capture only on the native branch with RR unselected, no partial RR, and Native RR Off mode; it never starts an RR capture from this button.

Each pair includes color entering native SR, depth, motion, native GBuffer1/2, raw diffuse GI, native accumulated diffuse/specular lighting, and the native SR output. No RR normal/albedo/specular guide generation or RR preset is required. SR output is still before final tone mapping, not a final display screenshot. This does not add a new simultaneous shadow-mask readback; R143 already supplies the signal comparison, while these images identify where normal-lighting contrast changes.

The export records evaluation_path=native_sr, rr_active=false, preset=0 and native-sr-output filenames. The collector validates these fields, complete resources, successful native evaluation and consecutive frames. Optional NGX pre-exposure/exposure-scale metadata carries explicit validity flags; these values are not assumed to be the engine's final adaptive-exposure value. Unsupported values are marked unavailable, never used as an exposure measurement.

Capture reuses the existing preparation, guarded state locks, source retention, exact barriers/restoration, fence retirement, background export and packaging. The native layout starts with scene color/depth/motion rather than RR guides; depth stays four bytes per pixel. No new rendering shader, GPU wait or render-thread file I/O. Accepted corrected modes remain available and unchanged; the new button replaces the rejected Baked GI Off shortcut with Native RR Off.

Validation includes native/ordinary builds, existing WARP resource state/lock tests, collector native-path rejection checks, UI/settling and capture/menu checks. These cannot establish in-game resource availability or RTX gameplay behavior. No shadow fix is claimed by this diagnostic build.
