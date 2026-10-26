# R161: capture the native spotlight projection

R160 MAP PRIORITY is rejected: the user reports lost legitimate shadows and the false floor edge remains. The overlay shortcut returns to SCREEN AO OFF. TRACE SHADOW always uses the accepted corrected RR OFF selector; internal legacy mode identifiers remain for compatibility, not as a proposed workaround.

The shader audit identifies the spotlight projection at bytes 80..143 of the 320-byte deferred spotlight record. The native selector reads columns from this matrix before projection division and atlas addressing. Shadow flags are at byte 304. Existing 128-byte ray context does not include either field. Stable emitter positions and hits several units before the source do not prove a source is inside a wall.

The bounded composition capture now copies the exact selected spotlight record from its existing t13 binding. It records selected ID, element count, stride and validity, then all 320 raw bytes. No new game resources, rays, shadow ranking, clipping or lighting math are introduced. The GPU record stride is 464 bytes: existing 128-byte before/after signal plus a 16-byte provenance header and 320-byte spotlight record. Export keeps the old signal line and adds a separate bounded rt_hit_gpu_light line to avoid oversized log messages. Existing fences and asynchronous export remain in use. Capture-only allocation increases by 1,376,256 bytes per signal buffer; four CPU/GPU allocations grow by 5.25 MiB total.

The decoder accepts old captures and requires exactly one matching valid spotlight record per exported ray when the new ABI is advertised. It preserves raw bits and decodes matrix rows and flags. The raw projection does not itself measure atlas visibility or establish geometry enclosure; further analysis must distinguish these limits. Receiver bias differs between map and ray paths, so any later projection of a ray hit is an approximate geometric test rather than an exact shadow-map sample.

## Test

Start a fresh session with usual DLSS and RT settings. Stand on the stairs with the false edge visible. Click TRACE SHADOW, hold still through the five-second settle and capture, and send lighting.log, manifest.txt and three PNGs. No MAP PRIORITY comparison is needed. This is a diagnostic build, not a claimed rendering fix.
