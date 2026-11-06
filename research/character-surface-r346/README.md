# R346 character surface isolation

R345 capture Volume-Light-20261105-220458-89528-0 completed with no failures.
Neutral participating media had 552/552 replacement binds; owned integration had
540/540. Background haze disappeared during isolation, but facial mottling
persisted. The images do not isolate temporal flicker because Jesse's idle pose
changes. Participating-media integration is therefore not the sole source.

The exact captured character.obj variant155 samples six input groups tested here:
- shadow buffer Y -> r0.x, multiplying one accumulated lighting term;
- shadow buffer W -> r0.y, multiplying the other accumulated lighting term;
- view-volume lighting normalized into rig tint/intensity, with brightness floor;
- diffuse lighting buffer t3;
- specular lighting buffer t4;
- translucency lookup t13 in each of three rig-light paths.

Do not infer semantic shadow-channel names beyond Y/W until these tests identify
the culprit. Neutral shadow channels become 1. Volume-driven rig modulation becomes
white with the existing brightness-buffer floor. Diffuse/specular inputs become
zero, separately. Translucency lookup results become zero. Each variant inserts
only the audited MOV instructions; original instruction stream, resource bindings,
material textures, final fog composition and alpha output are preserved.

Runtime variants are selected only during TEST JESSE. Original native PSOs are
always returned to the game; alternatives are owned via private interfaces on the
original object. Exact original PS bytes are required. All six PSOs must prepare
successfully before admission. No diagnostic shader matching runs per draw.
Counters measure recorded binds, not executed draws. A missing shader or missing
phase replacement makes the capture explicitly incomplete.

The one-click sequence contains native, six isolated inputs, native repeat. It
preserves selected RR/GI/lighting settings, uses full-client JPEGs, and restores
native character rendering after completion/cancellation/focus loss. Existing
R345 owned fog remains constant throughout. This is diagnosis, not a claimed fix.

## R347 correction
R346 capture reported matched=ready=binds=0 for every phase. No input-isolation conclusion is valid. R347 supports both audited native character pixel shader binaries (detail and basic material variants) and compares the complete SHEX, ISGN and OSGN chunks rather than reflection/debug container bytes. It never applies offsets to unknown live bytecode. Each family has its own generated replacements.

The capture checks readiness and recorded bindings before each screenshot and stops immediately on a failed phase. Failure packages bounded CPU shader candidates (at most 32 / 1 MiB) selected by character rig reflection name or matching declaration prefix. Candidate selection does not authorize substitution. No GPU readbacks are added. WARP checks both native and stripped inputs and rejects changed instructions. These checks do not establish game visual correctness or prove the actual cause of R346's missing match.
