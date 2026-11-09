# R108: GI sampling tests up to 16

Adds GI 12 and GI 16 to the F10 live diagnostic and the native graphics-menu Global Illumination Samples slider. Supported GI choices are Native, 2, 4, 6, 8, 12 and 16; the default remains 4. The game's separate Ray Tracing Samples Count slider is unchanged. This build raises GI sampling only, not specular/reflection or shadow ray counts.

Install with the game closed. Keep RR and native Ray Traced Indirect Diffuse Lighting on. In F10 compare GI 8, GI 12 and GI 16 at the painting and doorway while keeping the game's RT sample count fixed. Wait ten seconds each, note quality/FPS, and save one Lighting-R108 log. No restart between live comparisons.

The F10 controls temporarily override GI without saving the test value. BASELINE restores the saved GI setting; GI FOLLOW MENU follows the native count. To save 12 or 16 as a preference, use the native graphics-menu Global Illumination Samples slider and select BASELINE in F10. RR-off retains native sampling.

The existing producer hook applies the count before both native hit-buffer allocation and shader-provider binding. The audited native count range is 1..16. Mode changes use the existing history reset. Extra samples increase ray work and hit-buffer storage; this is a quality/performance test, not an optimization claim.

Centered filtered shadows, GI filtering, textured world albedo, skin/hair behavior and the fixed 60% clamp are unchanged. GI 12 and GI 16 replace the older CENTER FILTERED and VIEW ANGLE diagnostic buttons; all GI sample modes already include the centered filtered shadows.

Validation covers 12/16 admission, invalid-count rejection, RR-off preservation, live transitions, saved-setting restoration, UI schema generation and Windows shader/build checks. The shipped shadow, glass, guide and clamp shaders remain byte-identical to the previously verified assets.
