# R98: isolate RR-dependent projector-room speckles

R97 CENTER FILTERED is the user-confirmed shadow baseline. The projector-room comparisons are clean with RR OFF in both native and centered shadow modes and noisy with RR ON. This establishes RR-path dependence, not the particular faulty signal.

## Live test

1. Install beside Control_DX12.exe with the game closed. Preserve NVIDIA runtimes and existing ControlFGStreamline files.
2. Use the same projector-room camera. Keep RR ON and native RT Contact Shadows enabled; disable FG and glass enhancements for this comparison.
3. F10: select CENTER FILTERED, return to the game, and wait 5-10 seconds. Take a screenshot and record ISSUE REMAINS/GONE.
4. Select DIFFUSE FILTER, return to the game, wait 5-10 seconds, and repeat. This retains the exact centered, filtered spotlight path.
5. Switch back to CENTER FILTERED to check repeatability. SAVE LOG once and send that Lighting-R98 session log with the images. No restart between modes.

## Scope

Full RR ordinarily skips two broad diffuse DLF calls when the RT sun-shadow/AO effect bits are present and partial RR is inactive. DIFFUSE FILTER executes both original calls with their original arguments, including temporal feedback and spatial passes. This is a diagnostic restoration of prefiltering, not a proven final RR correction. If that bypass was already inactive, this test should not change the image.

The queued `rr_broad_diffuse` records identify each site, selection sequence, requested mode, RR selection state, effect mask, original pass count, actual completed native call versus bypass, and preservation request. A completed CPU call does not establish GPU RR evaluation or visual success. Existing RT-pair logs remain separate.

Native diffuse history may need a few frames to settle when restored after a bypass. This test does not modify history pointers, force GPU waits, add readbacks or allocate render resources. Render-side observations use the existing bounded queue; the UI thread writes the single session log.

All 14 spotlight shader assets remain byte-identical to R97. RR model F, fixed 60% reflection clamp, skin/guide code, GI hook, contact-shadow hook, glass source and FG provider remain unchanged. Other diagnostic modes retain their earlier meanings; use only CENTER FILTERED and DIFFUSE FILTER for this comparison.

## Validation

Policy checks cover every effect mask from 0 through 63 with full/partial combinations. Windows WARP checks route the new mode through the centered ray record, existing filtered reset/spatial/composition pipelines and R94 selector. These checks do not validate real-game image quality or RTX stability.
