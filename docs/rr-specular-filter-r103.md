# R103: native specular filter comparison

R102's captured painting and wall pixels have textured diffuse guides. The tested regions have valid world albedo throughout, and diffuse RGB matches that albedo. The reported stationary RR sharpening remained, so supplying albedo alone did not resolve it. This does not establish which remaining input causes the artifact.

R103 adds SPEC FILTER. Compare it with WORLD ALBEDO, with RR on, at the same wall/painting. Stand still for ten seconds in each mode, then move briefly to check trails and loss of detail. Save one log after both modes and include screenshots and observed frame rate. No restart between selections is required.

Both modes retain R100 GI filtering, R102 textured Standard-material albedo, and centered filtered spotlight shadows. SPEC FILTER additionally preserves Control's specular temporal history and its native spatial-pass count. WORLD ALBEDO retains the previous forced-camera-cut/60% clamp path and zero spatial passes. This is an isolated diagnostic of that combined specular filtering path, not a claim of a validated final RR integration.

Selection is latched before lighting. Transitions reset RR history; entry to SPEC FILTER clears native history once at the existing pre-filter boundary. Native filtering owns history allocation and normal camera-cut behavior after that. The experiment does not change normal, roughness, or specular-albedo guide formulas, add texture blur, or alter the shadow shaders.

The exact native temporal dispatch is checked against the current frame, view, device, queue, and source/target resources. It executes once; successful recording is required before the outer filter can report lighting ready. Missing, duplicate, invalid, or interrupted work cannot authorize RR evaluation. `specular_filter_validated` counts successful CPU command-recording observations, not GPU completion or proof of improved image quality. `specular_filter_rejected` counts rejected completed filter attempts. The existing log also identifies `native_specular_filtered` and the forwarded spatial-pass count.

No recurring readback, render-thread file write, GPU wait, or new hook is introduced. Native filtering adds GPU work and can soften detail or cause temporal trails; compare performance and moving objects as well as the stationary wall. The full RenoDX RR replacement add-on is incompatible with this native-filter comparison and is rejected. All fourteen spotlight shader assets and the glass shader assets remain unchanged from R97.

Validation includes frame/resource mismatch rejection, missing bind and duplicate-dispatch rejection, exact native dispatch forwarding, exceptions, unchanged raw-copy eligibility, live-mode policy, world-albedo admission, GI checks, Windows build and WARP shadow routing. These checks do not replace an in-game visual test.
