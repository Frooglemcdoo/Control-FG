# R77: locate the shadow failure in one live session

## Findings

R75's log confirms configurable rays and composition were submitted. This does
not prove correct per-pixel hits or geometry coverage. The user reports blobs,
weapon flicker, incomplete Jesse shadows and pop-in. Wall flicker is unresolved.

Code inspection identified three boundaries worth isolating:

1. The composition patch forces selected-light map visibility to one at SHEX
   DWORD 1707, before its zero-visibility gate. This removes map protection even
   if the ray scene lacks the relevant occluder. Light selection still changes
   by pixel. Abrupt light-selection/visibility boundaries are a hypothesis for
   patches, not an established diagnosis.
2. Native filtering derives blur width from emitter radius and blocker distance.
   It was built for short contact rays. Spatial passes check neighboring light
   IDs, but temporal history has no previous-light ID input. Restoring history
   alone does not resolve these signal assumptions.
3. The caster expansion modifies native scene collection. It does not establish
   that every caster or dynamic weapon is represented correctly. Native coverage
   is an independent test of whether that expansion contributes to weapon flicker.

External references support separating these stages, not any Control-specific
diagnosis: NVIDIA describes shadow filtering guides including hit distance,
depth, normals, light size and direction; NRD documents per-light SIGMA filtering.
Microsoft documents instance masks as a further independent geometry exclusion.

- https://developer.nvidia.com/blog/rtx-coffee-break-ray-traced-light-area-shadows-and-denoising-712-minutes/
- https://github.com/NVIDIA-RTX/NRD/blob/master/README.md
- https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#instance-masking

## Live test

Close Control only for installation; copy all package files. Keep the same game
settings throughout the comparisons. Use F10 buttons at the affected wall, then
repeat while approaching the light and watching Jesse's weapon. Return to gameplay
for several seconds after each switch. One Save Log at the end records all modes.

| Button | What changes from RT FILTERED | What to watch |
| --- | --- | --- |
| NATIVE SHADOWS | Original shadow path | Reference appearance and weapon |
| RT FILTERED | Full replacement with bounded history | Reproduce each issue |
| RT RAW | Bypasses spotlight temporal AND spatial filtering | Whether patches already exist before those filters; noise is expected |
| RT + MAPS | Restores native map visibility in selected-light composition | Whether retaining map occlusion removes patches or restores coverage |
| NATIVE COVERAGE | Restores native caster-expansion settings; RT remains active | Whether weapon flicker or pop-in changes with collection coverage |

Return to RT FILTERED between diagnostic modes. Record which mode changes each
symptom and send one R77 log plus a video. ISSUE REMAINS / ISSUE GONE records the
current mode; use separate video segments for separate symptoms.

RAW is raw spotlight visibility fed into normal lighting composition, not a
grayscale ray buffer. RR/upscaling can still process the final image. RT + MAPS
can hide missing ray occlusion and can over-darken shadows, so an improvement is
evidence about that boundary, not proof of a final fix. Native coverage can remove
off-screen casters; improvement there implicates collection but does not identify
which object is missing. None of these comparisons guarantees a unique cause.

## Implementation and checks

Both native ray variants remain supported and unchanged. Raw diagnostic PSOs
preserve point-channel operations and native resource layouts. Mode selection is
held across ray dispatch and composition; mode changes reset temporal history.
All modes retain pass/readiness guards and nonblocking logging. No synchronous
GPU readback, capture I/O, or extra render-thread wait is added.

Validation checks actual packaged raw instructions, runtime hashes, WARP shader
creation, diagnostic PSO selection, native coverage restoration, and pass scoping.
In-game results are still needed; this build makes no visual-fix claim.
