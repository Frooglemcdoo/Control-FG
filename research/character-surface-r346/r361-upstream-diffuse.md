# R361: restore upstream RT diffuse filtering

The user clarified that the remaining distortion is present with RT ON and absent with RT OFF. Earlier reports of RT-off character shadow noise concern earlier work; do not use them to classify the remaining defect as non-RT.

R360 run160616-83136-0 completed all twelve full-client images without failures, twelve admitted character PSOs, recorded skin/cloth/hair replacement binds, RR active and clamp60. Distortion remains in reconstructed full lighting and reconstructed base diffuse. Character animation prevents exact pixelwise before/after statistics. These JPEGs do not identify the exact light or prove the producer is defective.

## Mapping audit

Inspected the original diffuse load and preceding remap instructions in all fourteen available captured character bytecodes (twelve admitted families plus two reference copies). The map reads x from resource identifier10/slot11; pixel parity selects bit offsets8x+16y, sample index contributes2s, and the selected two bits replace x/y low bits. The native final integer coordinates retain screen-position fractional offsets before truncation. R360 follows the same integer mapping for nonnegative sample-count<=1 pixels. Native load output and coordinate registers do not alias in these shaders. No demonstrated mapping error was found. R360 MSAA>1 still uses the original sample, so that filter cannot improve MSAA>1.

## Implemented correction

The existing RR hook skips BOTH native broad diffuse DLF calls at0x2489a4 and0x24a3f2 when full RR is active, partial RR is inactive and RT sun-shadow/AO bits are set. The hook explicitly documents that these calls include temporal feedback even with zero spatial passes. R360 left this upstream bypass enabled.

R361 retains each original call and its unmodified color/history/pass arguments by default. It reuses Control's existing implementation, resources and history. It does not implement a new temporal denoiser, prove animation history validity, raise ray counts, or add another lighting producer. The existing R360 consumer gather and successful rig/near-history fixes remain the default so the initial comparison changes only the upstream bypass policy. Specular/RR clamp60, GI raw bypass, Owned GI selection and shadow routes are unchanged.

This restores previously skipped work and may cost GPU time. It is a candidate correction to an audited bypass, not proof of the final root cause. If the current RT configuration never visits these calls or the old policy did not skip them, it cannot affect that configuration. RT Indirect Lighting alone is not sufficient to activate the old bypass: the sun-shadow/AO effect bits matter.

## Compact verification

TEST JESSE retains RT/RR and runs four phases: R360 legacy bypass / upstream restored with R360 gather / upstream restored without consumer gather / upstream restored with gather repeat. Two full-client JPEGs each, five seconds settling per phase. Native callsite A/B, native completed-call and bypass deltas accompany existing shader/RR/clamp coverage counters. Counts are CPU observations and may include a transition call, not GPU completion, variance or performance measurements. Zero old bypass is explicitly not evidence of a tested restoration. All exits restore upstream preservation, including cancellation/focus loss/failure. No RR off/on transition.

## Validation

Portable policy truth table covers1024 combinations of all effect masks, full/partial RR, diagnostic priority and legacy comparison. The capture contract asserts RR read-only behavior, four phase plan, source evidence and unconditional cleanup. The exact two renderer edits are reversible in the clean-baseline audit. Existing84 shader variants are unchanged and retain Windows WARP/PSO checks. Game quality and GPU cost remain unverified.
