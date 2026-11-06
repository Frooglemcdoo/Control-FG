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

## R348 stage isolation
R347's completed capture had one matched/ready PSO and nonzero replacement binds in all six override phases. Lighting visibly changed, but the user reported noise throughout. This does not prove every tested source innocent: shared normal reconstruction and later processing remained.

R348 replaces the six TEST JESSE phases with:
1. Surface normal instead of bent normal: at original dword1266, r15.xyz = r2.xyw. All three rig light evaluations consume this same r15.xyz (1351/2376/3293). This is diagnostic, not a permanent normal/occlusion change.
2. Entire character rig off: zero r5.xyz and r2.xyz at3915, leaving accumulated diffuse/specular buffers r1.xyz and later fog.
3. Unlit textured albedo: output material/detail-modulated r3.xyz at609 and return, alpha1. No character lighting or fog.
4. Flat gray before fog: r0.xyz=.18 at3953, retaining analytical atmosphere and participating media; final alpha1.
5. Flat final: override output RGB=.18 and alpha1 at4751, after all character shading and fog. Later engine passes remain active.
6. Fog off: neutralize analytical atmosphere at4690 and participating media at4737, retaining character lighting.

Both audited shader families receive separately generated patches. Basic-family offsets after the material-detail section are57 dwords earlier, including609=>552. The generator verifies target instructions and lossless recovery of the original instruction stream. All overrides are temporary, preserve bindings, and use R347's readiness checks.

Interpretation: noise persisting on flat final points beyond the replaced character RGB/alpha (later processing, coverage/overlays or output), not specifically RR. A clean flat final but noisy flat-before-fog implicates fog composition inputs. Clean unlit albedo but noisy lit variants keeps lighting/normal reconstruction in scope. Idle animation and changed brightness prevent exact per-pixel comparisons. User observation is needed for flicker; stills alone cannot prove temporal stability.
