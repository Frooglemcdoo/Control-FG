# R419: motion instability research and one concrete correction

## Locked problem and constraints

The remaining jaw/neck noise is reported during movement. Red/orange speckles
flare on hair/face during turns. A stationary capture is not an acceptance test.
Preserve the prior accepted projector, desk and plant appearance. RELAX softness
and halo regressions were rejected. Do not restart global clamp/spatial tuning,
repeat character-input isolation suites, or request another generic capture.

R418's supplied runtime log proves four skin record creations (skin-basic and
skin-live-noise-b) and two hair-opacity record creations with diagnostics off.
This eliminates missing replacement creation for those variants in that run;
it does not prove exact pixel coverage or correct temporal correspondence.

## Research findings

1. NVIDIA's RR guide requires dense motion including dynamic objects and depth
   consistent with the motion-vector producer. Valid formats, finite vectors,
   matching frame IDs and successful evaluation do not establish correspondence
   to the same moving skin/hair surface. Our native-NGX gateway forwards Control's
   primary motion/depth. This review does NOT establish that those vectors are
   wrong and does not replace them with camera-only reprojection.
2. NVIDIA's hair guidance explicitly addresses unreliable screen-space motion on
   thin animated strands using previous/current geometry. Control's hair cards
   are not RTXCR's LSS primitives; copying the sample's strand motion code would
   be unjustified. The transferable requirement is coherent animated-surface
   tracking, not smoothing all normals or blending motion vectors.
3. NVIDIA's reference renderer deliberately preserves temporal history during
   ordinary camera/object motion. Resets discard accumulation and add noise.
   This led to the concrete source defect below.
4. Specular motion or hit distance is documented for reflection motion. This is
   not a new proposal here: R190 already explored compact hit distance and later
   work retained its off baseline. Missing reflection motion alone does not
   explain the jaw issue or justify repeating that experiment.

## Reproduced defect: content visibility drives a global reset

`rr_live_guides.h` publishes `rrWorldAlbedoGuideFrame` only if a same-frame
textured replay contains world batches. In `rr_evaluation_entry.h`, the previous
code fed that frame-presence boolean to `worldHistory.Update`. A false/true
transition reset all RR history, including unaffected character pixels.
`viewAngleHistory` similarly tracked `bindings.viewAngleApplied`, which includes
the visible-world-batch condition. The current Native Blend default enables
world albedo; the perspective comparison is a separate non-default mode.

The executable regression uses the production history class and mode predicates.
For fixed mode and coverage `1,1,0,1,0,0,1,1`, it reproduces five old resets versus
one initial reset when tracking mode configuration. Real option transitions
still reset. The test also guards the production call sites and retention of the
other reset sources. This is a synthetic trigger reproduction, not evidence of
five resets in the user's video. R418 logs did not include per-frame reset causes.

## R419 correction and boundaries

The two reset inputs now track world-albedo and perspective-guide mode selection.
Visible surfaces entering/leaving the replay no longer establish global epochs.
The change intentionally preserves RR's normal local disocclusion handling.
Native camera-cut/reset, extent changes, preset changes, reflection-clamp changes,
hit-distance binding changes, readiness validation and failure paths remain.
No shader instruction, light intensity, sample count, material guide value,
spatial kernel, GI history, motion vector or depth value changes.

This corrects a reproducible integration defect. It is NOT a confirmed root cause
of the submitted jaw/hair video. If world-guide presence stayed true throughout
that movement, this particular correction cannot explain or fix that run.
Do not relabel a passing build/fixture as a completed visual fix.

## What further source work should establish

Audit previous-pose/skinning participation in Control's native velocity producer,
and its hair-card coverage relative to replay/depth. Match any conclusion to
actual shader/producer evidence, not merely nonzero motion. Separate this from
noise already present in character lighting: earlier RR-off and composition
tests prevent a blanket claim that RR creates all neck noise. No new user run is
requested by this research note. Any future acceptance must involve movement.

## Primary references, read November 10, 2026

- NVIDIA Streamline RR programming guide, sections 4.1.3, 4.1.6-9 and 4.2:
  https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md
- NVIDIA RTXCR hair animation denoising:
  https://github.com/NVIDIA-RTX/RTXCR/blob/main/docs/RtxcrHairGuide.md
- NVIDIA reference renderer, Jitter & reset:
  https://github.com/nvpro-samples/vk_gltf_renderer/blob/main/docs/denoising.md
- NVIDIA RTXPT design, vertex processing and denoiser guide generation:
  https://github.com/NVIDIA-RTX/RTXPT/blob/main/Docs/Design.md

The current Streamline page is version 2.14.16; this project has used 2.14.1.
R419 does not adopt new SDK APIs or change the installed RR runtime.

## Verified package

Runtime commit `72e47e8daeb17c7f33266cf39c11edb1e466b9a9`.
Windows workflow 42926804776, job 130215229495: all 60 steps succeeded.
The Windows log confirms the visibility-reset regression and 132 protected files
passed. Downloaded ZIP CRC and published SHA, x64 DLL, build labels and startup
policy marker verified. Package has 83 entries and 6,797,164 bytes.

ZIP SHA256: `c6dc244de59c60e19a83452cfaf385a55c60d882f8230a0e31965413a65c5e79`.
DLL SHA256: `4e9d61ccb86ae26c0bb6f21a1279115b88eef20cdc58562eeecd7691ce4c84f8`.
No game-side movement result has been obtained for this correction.
