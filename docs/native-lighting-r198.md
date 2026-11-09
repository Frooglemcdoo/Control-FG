# R198: identify window blockers excluded from native RT shadow rays

The three R197 captures completed successfully. At 270 grid locations the
native map blocked the selected light while RT let it through; full-length
reference rays also missed. That supports a caster mismatch but does not prove
out-of-bounds culling. R198 adds two geometry-only probes for each reference miss.

## Capture

Keep Native Blend, RR ON, SPEC Native, compact hit distance OFF, and the same
RT settings. At the affected window use F10 -> TRACE SHADOW and hold still.
Send lighting.log, manifest.txt and the three PNGs from Shadow-R198.

One capture batch records the original mask-1 ray with native alpha handling,
then two inline triangle queries for every miss: force-opaque with mask 1 and
force-opaque with mask 255. The queries use exactly the same TLAS, origin,
direction and full endpoint. They do not use or widen the native hit-table
indexing. The displayed lighting, RR, sample count and R193 reflection fixes
remain unchanged. No noclip/free-camera control is included in this build.

Possible results:
- Geometry hit with mask 1: the normal material/opacity path rejected a blocker.
- Mask 1 misses but mask 255 hits: the broader mask reveals excluded geometry.
- Both miss: no triangle was accepted even under those settings. Missing or
  culled geometry remains a possibility, but mask-zero instances, different
  shadow-map caster meshes, procedural geometry and endpoint/path differences
  are not ruled out. This is not proof of a room-bounds problem.

The all-mask query may hit geometry intentionally excluded from shadows.
Force-opaque may hit glass or alpha-cutout surfaces that should transmit light.
Neither query is suitable as a blanket gameplay replacement.

## Bounds and verification

Extra queries run only during the requested Native Blend diagnostic, only on
reference misses: at most 8192 extra queries for the 4096-point grid. Two extra
80-byte result banks add 640 KiB of GPU result storage and corresponding CPU
readback storage. Existing fences retire all three banks together. R197's
ray-table warmup and map/composition provenance checks remain enabled.
Inline ray queries require DXR 1.1 support. A failed capture pipeline does not
replace the normal lighting pipeline.

Windows CI compiles and validates the DXIL, exercises the real export queue
retry path and existing resource/submission tests, and tests decoder rejection
of stale, missing, duplicate, mismatched or invalid probe records. Actual DXR
scene intersection behavior must still be validated in-game; WARP tests do
not execute this ray-tracing scene. tools/analyze-window-blockers.py analyzes
the result and rejects older captures lacking the new probes.
