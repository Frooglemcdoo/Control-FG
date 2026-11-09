# R162: projection-origin RT comparison

R161 captured 4096 valid records with zero failures. The spotlight record from composition matches the independently captured ray context exactly in its first 12 floats. Inverting each projection places its center at the cone apex, not the nominal emitter. For the false-edge light the offset is 0.652704 engine units; sampled RT paths differ from projection-center paths by up to 0.3603 units at blocker height. This establishes an authored source-position difference, not that the light is inside a wall or that this causes the false edge. Atlas visibility is still unmeasured.

PROJECTION RT changes only the center target of the four spotlight segments to the per-light cone apex transformed by the native inverse view. This matches the projection center for the sampled lights; it is not a general runtime matrix inversion for every light. Existing centered disk offsets, receiver bias, authored plane clipping, hit/miss and alpha-test handlers, point branch, filters, composition, light ranking and ray count stay intact. The same offsets/filtering do not guarantee identical apparent softness after changing direction/distance; visual verification is required.

Compare RR OFF and PROJECTION RT in the overlay. Both bypass RR and use current-frame spotlight history with the accepted normal/GI corrections. Default remains unchanged. The projection shader is optional: linking failure retries the accepted shader set without it, and the button remains unavailable. MAP PRIORITY remains rejected and is not used.

Stand on the stairs with the false edge visible; record the same view in both modes, then walk down. Check Jesse, railings, close shadows and previously fixed rooms. Click SAVE LOG and send the session log and comparison video. TRACE SHADOW still captures the accepted RR OFF reference; do not interpret its records as projection-mode rays.

Validation includes exact hash-locked R97 input, reversible 12 center substitutions plus the new coordinate transform, unchanged authored-clipping transform, signed DXIL validation, mode policy/readiness and ray-record bounds. The live driver must validate visual correctness and performance.
