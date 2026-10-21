# R70: four-sample spotlight visibility

R69 stopped the user-reported freezes. Keep that logging/UI fix. Remaining
reports include soft-shadow dithering, flicker, and larger coverage changes.

R70 replaces the single spotlight sample with four symmetric samples on the
same native disk. It averages visibility within the current frame before the
existing temporal/spatial filters and R68 history clipping. It uses the nearest
blocker distance for native spatial filtering; miss-only pixels retain the
native sentinel. Point rays, light radius calculation, masks, alpha handlers,
geometry selection and composition are unchanged.

The four samples share a radius and are correlated: this reduces some sampling
variance but is not equivalent to four independent rays. The expected disk
sampling distribution is retained. Soft edges remain sampled, not replaced by
hard center rays. GPU cost increases by three spotlight rays per valid pixel;
measure FPS as well as quality. No claim of a measured performance cost yet.

This is not a caster-coverage fix. Missing geometry and changing selected light
IDs remain possible causes of larger pop-in, and need further investigation if
those symptoms persist. Do not treat all pop-in as sampling noise.

Close the game and install all files. Test RT SPOTLIGHTS in the stairwell and
the distorted hallway, stationary and moving, with RR on. Compare native using
F10 without restarting. Save Lighting-R70 and report dithering, pop-in and FPS.
