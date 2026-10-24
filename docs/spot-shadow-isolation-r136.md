# R136: separate native lighting from RR reconstruction

The R135 bundle `Shadow-R135-20261024-082303-44772-0` contains the forward dark patch in Native, Default, No History and Selected Light Open. All those full-color screenshots had RR selected (`rr=1`). Native means no diagnostic lighting override, not a clean RR-off game comparison. The first test must therefore separate reconstruction from shadow production before attributing the patch to AO, contact shadows, or another light.

The strict GPU decoder accepted generation/frame 596510, extent 2560x1440, 4096 valid rays, 1731 hits and 2365 misses. Every sampled receiver selected spotlight 27. The table contained 9471 translated and 9471 null records, with zero unknown identifiers, invalid layouts or invalid result records. This is the selected spotlight, not an inventory of all point/spot/fill lights. The capture does not establish a second source or identify the remaining patch's cause.

## Changes

- Keep Native / Both / Default (Clip) on the top row.
- Add Native / RR Off and Clip / RR Off as full-color comparisons. These use the existing per-frame RR bypass and leave the saved RR preference intact. Color diagnostics and full-color RR-off comparisons now have separate predicates.
- Add Light Off: omit the selected spotlight's additive lighting while retaining the authored ray boundary. This is diagnostic isolation, not a replacement shadow solution.
- F9 now captures all three additional views automatically: 16 steps, up to 48 PNGs, and one GPU hit/light-context capture. It waits for RR to be reported off before taking RR-off images.
- Fix GI Only's false binding wait. GI Only intentionally does not run the replacement RT pass, so it cannot produce that pass's observation marker. Like Native it waits for elapsed time and rendered presents. Modes that actually replace the RT pass still require its marker.

The authored-clip shader remains byte-identical to R134. Spatial/history policies and material guides are unchanged in Default. No AO/contact-shadow removal is claimed or applied here. R135's no-history comparison did not establish temporal history as the cause.

## Test

Install with the game closed, then return to the same spot. Leave RR on in the game's settings so the normal and forced-off comparisons differ. Keep FG off for diagnostics. Press F9 and keep the camera still until the completion dialog; allow about 60–90 seconds. Send the resulting Shadow-R136 folder as ZIP or 7z. The suite restores the selected mode and the frame policy restores the saved RR preference. F9 again cancels.

Manual comparison: Native against Native / RR Off, then Default against Clip / RR Off. If the patch survives both RR-off views, the evidence supports moving on to native occlusion/contribution isolation. If it disappears, investigate RR inputs/history rather than suppressing a legitimate shadow source.

Images are display-referred game-client screenshots, not raw HDR buffers. Hit-position/visibility views remain unclipped reference diagnostics. All skipped, timed-out or failed steps appear in the manifest.

## Validation

Tests cover RR-off mode routing without color-debug output, saved preference and resize-pause preservation, authored shader routing, GI-only readiness without a replacement binding, and rejection of RR-off captures while RR is still selected. The Windows build also runs the existing GPU pipeline and repeat-capture retirement tests.
