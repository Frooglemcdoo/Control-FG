# R137 — contact visibility and screen occlusion

R136's supplied capture completed all 16 modes (48 images) with zero failures.
The dark region around Jesse remains with RR bypassed and with the selected
spotlight visibility forced open. This does not identify an extra light or prove
an AO defect. R137 separates two remaining shading inputs.

## Live comparisons

Keep the same scene, camera and game settings. F10 exposes:

- **Default (Clip):** unchanged R134 authored clip, centered shadows and GI filter.
- **Contact Off:** neutralizes the selected spot **and point** RT visibility in
  composition. This deliberately removes their RT shadows for diagnosis; it is
  not a proposed visual fix. The ray pass still runs, and map/cookie/material
  calculations remain unchanged.
- **Screen AO Off:** neutralizes the `g_tShadowBuffer.yw` occlusion inputs in both
  the direct-light/ID producer and selected-light composition. This is scoped to
  direct-light screen occlusion, not every AO/GI effect in the game. Bent normals,
  material-authored occlusion and the sunlight's separate shadow channel remain.

Native, Both, both RR-off comparisons, Light Off and Light IDs are retained.
No History and Clip Raw remain in the automatic suite.

## Capture

Press F9 at the affected spot and keep the camera still until completion, roughly
90 seconds. The 18 comparisons produce up to 54 PNGs plus GPU hit/light context
and one log. F9 again cancels; focus loss cancels; the previous mode is restored.
Zip the displayed `Shadow-R137-...` folder. No restart between modes is required.

Screen AO Off must observe **both** modified PSOs before taking screenshots.
Unavailable variants are explicitly skipped. Optional creation failure cannot
turn off the working Default route. RR preferences and engine options are never
written; RR remains on if the user's selected baseline has it on.

## Implementation and verification

Three optional embedded DXBC variants each replace exactly one eight-DWORD load
with a scalar-one move to the same destination lanes and padding NOPs. Inputs
are locked to full SHA256 and exact instruction words. Container lengths,
resource declarations, material math and every other instruction remain intact.
No shader sidecars, ray libraries, sample counts, histories, skin guides or glass
assets change. New modes reuse the authored-clip ray record at offset 384.

Local checks decode destinations, lane masks and unaffected loads, reject altered
inputs, and verify two-pass capture readiness and mode storage. Windows CI also
validates shader disassembly, WARP PSO creation and runtime routing. These checks
cannot establish the cause of the user's shadow or prove a visual correction.
