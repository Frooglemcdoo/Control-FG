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

## R349 full-character scope correction
R348 matched only the skin path. Its clean gray skin and cleaner rig-off face do not establish a result for clothing or hair. R349 admits six separately audited shaders (two skin, two cloth, two hair) by exact executable/signature matching. Alpha and depth outputs, including hair depth offset, are retained in every phase.

TEST JESSE now runs both shadow-mask channels neutral together, rig off, rig only (remove the existing diffuse/specular-buffer contribution), gray before fog, gray final, and fog off. Every phase requires nonzero recorded binds for each of skin/cloth/hair and nonzero substitutions for each override phase. Counts are recorded PSO binds, not executed draw proof; visual gray coverage remains essential. Missing coverage stops and packages shader candidates.

These shaders use the same Y/W shadow mask channels but different material/light accumulation registers. The generator derives insertion boundaries only from SHA-verified originals, asserts the complete local composition sequence, preserves all original instructions, verifies reversible recovery and resource bindings, and creates a separate replacement set for every shader. Windows checks create native and stripped PSOs with all six replacements for each family.

This is full-character localization, not a claimed repair or a shipping shadow-disable workaround. Preserving the authored lighting while fixing its unstable source remains the goal.

## R350 captured-variant support
R349's failure report observed208 graphics pipeline descriptions, captured9 candidate binaries with no drops, and recorded only the skin group. No comparison images were taken. The already-supported basic skin hash matched. Six additional captured binaries are now admitted separately: skin live, cloth live, hair with native noise/coverage inputs, two skin noise variants, and cloth noise variant. Their resource bindings and shader instruction sequences were inspected against the existing material implementations; each replacement derives from that exact captured original. Two remaining candidates with different material math are retained in the original report and not admitted by similarity.

Family count and coverage groups are explicit generated metadata, not assumptions based on array order. The validator creates every original/stripped family plus all six replacements using a root signature covering the captured sampler0, sampler6, sampler19,8 CBVs and24 SRVs; production continues to copy each game's actual original PSO/root signature. All native noise, alpha, sample coverage, and depth code remains intact. This corrects recognition only; full-character quality is still unproven.

## R351 evidence and candidate correction
R350 capture Character-Surface-20261106-083159-57852-0 completed all eight phases,
matched/ready9, zero failures. Skin, cloth and hair had recorded replacement binds
in every override phase. Full-character gray views confirm broad visible coverage.
Rig-off substantially reduced the obvious skin/neck/jacket mottling; rig-only,
shadow-channel neutral and fog-off retained it. Stills and idle pose changes do not
establish temporal stability or an exact defective instruction.

The rig samples view-volume RGB and scalar intensity, normalizes RGB by luminance,
applies a brightness floor, then shades its authored lights. Skin and cloth also
consume screen-space bent normals; the admitted hair shaders do not. R347/R348
isolations were skin-only and do not settle interactions across the whole character.

R351 offers FILTERED/NATIVE in F10, FILTERED at startup. The candidate replaces
only those two character-consumer volume reads with eight trilinear sample pairs
at XYZ half-cell offsets. It averages coefficients before the unchanged nonlinear
normalization/floor. Actual grid dimensions come from native constants. Normalized
weights preserve constant inputs; no new temporal history, random jitter, render
pass, GPU resource, CPU readback or light removal is added. Existing material,
alpha, coverage, depth, bent normals, rig BRDF, shadow masks and fog remain native.
Cost:14 extra texture reads per covered character pixel and5 temporary registers.
A one-cell neighborhood may soften volume transitions. Game performance/appearance
are unverified; this is a candidate, not a proven fix. This changes the character
consumer, not the previous participating-media integration or producer filter.

TEST JESSE now captures native, candidate filter, surface-normal substitution,
uniform volume modulation, both substitutions together, rig diffuse-only,
rig specular-only and native repeat. Buffer lighting remains in the latter two;
only the rig's other lobe is zeroed. Hair's surface-normal phase is intentionally
unchanged because that family has no bent-normal input. Diagnostic substitutions
never persist. Completion/cancellation resumes the prior FILTERED/NATIVE choice.

The Python test executes the emitted filter instructions on360 synthetic cases,
including both native channel layouts, grid sizes, constant/random/checker inputs
and clamped edges. Exact downstream instruction recovery and bindings are checked.
Windows WARP validates native/stripped PSO creation for all12families and6variants;
these tests do not render a real character or prove flicker removal.

## R352 HQ reconstruction
User reports R351 is a massive improvement but residual shimmer/flicker remains.
Video20261106-1523-51.8870291.mp4 is11.47s/30fps. Inspected overview and consecutive
frames show remaining fine variation around cheek/neck and jacket shading during
idle motion. Compression, pose and downstream shading prevent assigning every
remaining temporal change to one shader input from this video alone.

HQ evaluates a cubic B-spline convolved with a [1,2,1]/4 kernel on each volume axis.
The6x6x6 positive voxel weights are factored into27 trilinear reads per texture.
This extends Sigg/Hadwiger's adjacent-pair factoring:
https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-20-fast-third-order-texture-filtering
The plain4-tap cubic alternative was not shipped: although it smooths cell
transitions, its narrower kernel would preserve more high-frequency input noise
than R351. The final wider kernel preserves constant energy, has no negative
lobes/overshoot, and supplies continuous first/second derivatives for a fixed
input grid. It filters only the rig's volume RGB/intensity before the existing
normalization. Material normals, texture detail, rig lights, BRDF, shadows,
alpha/depth/coverage and fog remain native. VL High remains supported.

Cost:54 texture reads (27 per volume), versus16 in R351 and2 native. Twelve extra
temporary registers versus5 in R351. No extra GPU pass/resource, readback or new
history. This has real character-shader cost and may soften volume transitions;
no game FPS or complete temporal-stability claim is made. Source-volume changes
and other temporal effects can still survive a spatial reconstruction filter.

F10 RIG cycles HQ(default), native, R351. R351 generated binaries are byte-identical
to the prior build for all12families. Only2 replacement PSOs per family are now
prepared, down from6. TEST JESSE is four phases/eight images: native,R351,HQ,native.
Capture overrides selection temporarily and preserves it across completion,
cancellation and failure. Skin/cloth/hair coverage remains mandatory.

Validation:960 emitted-shader evaluations against an independent direct216-voxel
reference and R351 reference, constant/random/checker/impulse fields, multiple
volume dimensions, boundaries and derivative continuity. Stationary synthetic
uncorrelated volume noise RMS0.025814(R351) versus0.014429(HQ), not a measurement
of the user's game. Native downstream instructions recover bit-for-bit. Windows
checks create both quality variants for all12native/stripped families and verify
quality/capture selection. In-game quality, performance and remaining temporal
noise must still be evaluated.

## R353 guarded near-volume history candidate
R352 video20261106-1604-47.3172574.mp4 and user feedback: much better, a little
flicker remains. R352 character reconstruction is unchanged byte-for-byte.
The audited native deferredlight_vvfilter shader gates history using
`g_fViewVolumeFilterTemporalWeightMinDistance` against the current depth-slice
index (r1.z); near slices use current coefficients only. The video cannot prove
that this gate is the remaining source, and its live threshold is not captured.

R353 adds conservative reuse only where the native depth gate disables it.
It uses the existing reprojected history, capped at min(engineWeight,0.75).
A uniform weight over both coefficient textures fades to zero when any channel
changes by25% relative to max(abs(current),abs(history),0.001). Unclamped old
XYZ outside[0,1] and nonpositive old clip W reject near history. Engine weight0
still disables history (including resets); far slices keep native weighting.
Rejection explicitly selects current values, avoiding zero-times-NaN pollution.
This is coefficient-change rejection, not object-ID/depth/normal visibility
validation; small moving changes can retain a short lag. No claim of perfect
flicker removal or complete disocclusion rejection is made.

The shared view-volume pass is changed, so other near-volume consumers may be
influenced too. Character normals/materials/rig BRDF/shadow inputs and R345 fog
integration remain unchanged. Four native volume reads remain four; seven extra
temporaries and arithmetic in the existing compute pass, no extra pass, resources,
CPU readback or per-frame shader analysis. Native PSO stays owned by the game;
exact SHEX admission accepts stripped reflection containers, unknown code fails
closed. F10 RIG cycles STABLE(default), NATIVE, R351, R352; WAIT means no recorded
replacement binding has been observed. Counters do not prove executed GPU draws.

TEST JESSE compares native/R352/STABLE/native and resumes the chosen quality.
STABLE phase requires history replacement binds as well as all three character
groups. Eight full-screen JPEGs remain a compact appearance diagnostic, not a
complete temporal measurement. Live comparison requires several frames to let
existing history settle after a toggle; no artificial history clear is added.

Validation: structural checksum/resource/reprojection tests and unchanged960
character-filter evaluations locally; ten WARP dispatch cases in CI cover near
reuse, native far weighting, global reset, XY/Z bounds, large lighting changes,
constant-field energy and low engine history weight. Actual game confirmation
and GPU performance remain outstanding.

## R354 neck input isolation and startup clamp60
User confirms R353 fixed the prior flicker. New video20261106-1653-27.4116257.mp4
shows dense mottled neck/under-jaw noise at6.97–7.03seconds while the camera rotates.
This cannot identify shadow/translucency/bent-normal/buffer input from video alone.
R354 is a targeted diagnostic build, not a claimed repair of that remaining noise.

User corrected startup clamp request from50 to60. requestedMode now Override60;
compiled clamp remains60, Native/Bypass remain available and effective tracking
still reports actual admission. The existing suite policy verifies default60,
explicit Native, Bypass and capture overrides. The historical baseline guard
reverses only this exact authorized initializer change before checking old hashes.

TEST JESSE now compares R353 stable / shadow Y neutral / shadow W neutral / skin
translucency off / surface normal instead of bent normal / rig off / diffuse and
specular buffers off / stable repeat. All phases retain EXACT R352 HQ volume
reconstruction and R353 near-volume history. Sixteen full-client JPEG95 frames;
no giant resource dump. Previous quality resumes on completion/cancel/failure.
This is source isolation; altered lighting only exists during capture. The
translucency phase is a documented no-op for cloth/hair (no such input); the
normal phase is a documented no-op for hair (no bent-normal input).

The six diagnostic insertions are derived independently for all12 audited shaders.
Each insertion is reversible back to the byte-identical HQ shader. Original alpha,
coverage, depth, output writes and descriptor bindings are retained. Unknown
shaders are never patched. Group metadata moves to GUID index16 to avoid collision
with new variant index6. There are8 replacement PSOs per native family instead
of2; startup PSO preparation/memory grows, ordinary gameplay remains the same
single selected shader. Capture phase counters now cover0..11. All capture phases
require history replacement binds and all three character groups; unsupported
input no-ops are explicit in the audit, not evidence that an input was changed.

The short still sequence localizes visible mottling only if it reproduces during
capture. It cannot guarantee full temporal diagnosis during arbitrary camera
motion. No claim that this clip proves an additional defective shader is made.
