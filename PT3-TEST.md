# PT3 — Hybrid Path-Traced Beauty

PT3 is the first non-diagnostic Path Tracing view.

The proven PT2 R2 TLAS access, proxy-aware device identity, private FP16 output and late SDR/HDR presentation composite remain intact.

## F10 Path Tracing controls

- **PT OFF** — true native Control RT/RR baseline. No PT3 dispatch and no PT3 composite.
- **PT ON**
  - **BEAUTY** — default PT3 view
  - HIT/MISS
  - DISTANCE
  - INSTANCE

PT3 uses a new `ViewPT3` setting so older PT2 debug-view persistence does not force a diagnostic view on first PT3 launch.

## BEAUTY v1

This is deliberately a hybrid path-traced lighting implementation rather than a claim of a finished fully material-resolved multi-bounce renderer:

- primary surface visibility/material context comes from Control's raster GBuffer1/GBuffer2
- primary material reflectance uses the authenticated MaterialDataPart1 buffer when available
- primary world normal and roughness use the already validated Control G-buffer decoder
- one stochastic cosine-weighted diffuse secondary ray is traced through Control's `g_rtScene`
- one roughness-driven stochastic specular secondary ray is traced through the same TLAS
- sky/environment miss radiance and bounded hit radiance are combined with the primary material/Fresnel response
- output is written to the private FP16 PT texture and composited at the proven final-visible presentation boundary

PT3 does **not yet** reconstruct the full Control material/texture BSDF at arbitrary secondary hits. That is the next renderer step after this beauty path is stable.

## Expected test

1. Launch with normal RT/RR.
2. Open F10.
3. Leave PT OFF and note the native scene.
4. Turn PT ON. BEAUTY should be selected by default.
5. Compare PT OFF <-> ON from the same camera position.
6. Use the diagnostic views only if needed.
7. Send compact logs.

Expected runtime markers:
- PT3_TOGGLE
- PT3_READY
- PT3_DISPATCH_OK ... mode=beauty
- PT3_COMPOSITE_READY
- PT3_COMPOSITE_OK ... mode=beauty
- PT3_COUNTERS ... diffuse_hits/diffuse_misses/spec_hits/spec_misses
