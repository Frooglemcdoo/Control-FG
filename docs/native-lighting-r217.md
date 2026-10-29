# R217: depth-aware current-frame GI smoothing

Default: Native Blend, GI CLAMP ONLY, SMOOTH ON, GI VIEW NORMAL. RR stays enabled. This is an experimental response to the R216 captures, not a confirmed ghosting fix.

## Test

Use RR ON, FG OFF, SPEC Native and hit distance OFF. At the same projector-room view:

1. Test the default SMOOTH ON while stationary, then walk and pan past Jesse and the chair.
2. Toggle SMOOTH OFF in Clamp Only and repeat. This is the R216 Clamp Only reference.
3. Select GI BOTH and repeat as the clean-but-ghosting reference.
4. Return to Clamp Only / Smooth On, SAVE LOG, and send the log with a short movement video. Report speckles, dark-area boiling, ghost trails, halos, and any FPS difference.

Keep GI VIEW NORMAL. Mask and Zero retain the older R215 diagnostic meanings and do not visualize the new smoother. All four GI stage selections remain available. SMOOTH applies only to Clamp Only / Normal.

Optional raw capture: CAPTURE RR first in Clamp Only / Smooth On, then in Both at the same stationary view; wait for Saved after each. PACKAGE RR produces GI-Boundary-R217 ZIP parts. This sequence now validates the smoother-on capture as its first pair.

## Implementation

The new variant runs in the existing audited temporal-filter dispatch, with native history weight zero and no native spatial passes. It averages a 3x3 neighborhood with binomial weights (center 4, axial 2, diagonal 1). Each current-frame neighbor receives the native mip-based luminance clamp while retaining its RGB ratios. Samples whose current depth differs by more than 1% of center depth are rejected; center weight remains nonzero. This preserves uniform lighting and protects depth silhouettes, but cannot protect every same-depth illumination/material edge. There is no normal input in this shader's binding contract.

The pass adds 25 texture samples in the existing dispatch, with unchanged resources, root signature, output format and engine shader records. No CameraCut override, history clears, or extra full-frame intermediate. RR still uses its own history. Missing smoother PSO falls back to the existing no-history Clamp Only PSO. Actual performance and motion stability need in-game testing.

R216 evidence: all nine uploaded archive hashes and 67 reconstructed files verified. Both pairs contain consecutive successful RR evaluations, reset=0, matching-frame filter observations. Filter resource IDs match raw-gi.rgba16f in all four frames. In the normalized floor ROI x=.51..58, y=.70..86, first-frame luminance high-pass RMS (Gaussian sigma 1 pixel) was .00035744 in Clamp Only versus .00008070 in Both, about 4.4x lower. The GI and diffuse accumulation pixels match in the sampled floor region. Specular signals remain in both modes. This supports smoothing dense GI variation rather than only capping isolated peaks; it does not establish motion-ghosting causality from stationary pairs.

Validation includes shipped-bytecode checks, unchanged baseline hashes, WARP dispatch/restore and fallback tests, and pixel-reference tests for flat/black lighting, checker noise, bright peaks, thin lines, depth edges, camera cuts, and bright/dark history independence. Production branch remains unchanged.
