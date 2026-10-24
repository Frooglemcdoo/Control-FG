# R140 — motion shadow isolation

The user confirmed R139 fixes the broad false shadow with no observed regressions.
The next report is flicker around the stairs and railing while the camera moves;
it is not visible when standing still. The supplied 22-second video is evidence
of view-dependent changes, not proof that one particular screen-space pass is
responsible. No new universal fix is claimed yet.

## Baseline and live comparisons

Default (Fixed) remains byte-identical to R139's surface-normal shaders, authored
clip rays and shadow filters. Native and Both keep their existing meanings.
The overlay's second row now isolates one feature at a time **on top of R139**:

- **Screen AO Off**: neutralize direct diffuse/specular scalar screen occlusion
  (shadow-buffer y/w) in both light selection and composition. Preserve corrected
  normals, sunlight's separate x channel, maps, RT visibility and filtering.
- **No History**: keep the corrected producer/composition and spatial softness,
  but use current-frame spotlight visibility instead of spotlight history.
  Point history stays native. More noise is possible; this is an isolation test.
- **RR Off**: preserve the default lighting and temporarily bypass RR without
  changing the saved RR preference.

The third row offers **Raw Shadows** (corrected default without spotlight history
or spatial filtering), **Light IDs** (the older unclipped diagnostic color view,
not a full-color corrected comparison), and **Clip Only** (pre-R139 comparison).
Raw Shadows can be noisy; it is not proposed as the final quality setting.

## Test movement, not only screenshots

Install with the game closed. Load the same stairs. With RR and graphics options
unchanged, repeat the same short camera movement around the railing in Default,
Screen AO Off, No History and RR Off. Close the overlay before walking. Record
which mode changes the flicker using Issue Remains / Issue Gone, then Save Log.
Send a video including the mode switches and that single session log. If the
first comparisons all flicker, try Raw Shadows and Light IDs as well.

F9 still runs the complete stationary suite (24 modes, up to 72 PNGs) and restores
the chosen mode. It includes the corrected comparisons but cannot establish
motion stability from still images. No automatic camera movement is introduced.

## Why these comparisons

The old AO/history tests did not retain R139's normal correction, so reusing them
would change more than one feature at once. The new AO shaders combine the exact
validated normal edits with only the existing scalar-occlusion load replacement.
The native spotlight temporal pass has depth/motion validation but no previous
light-identity input; this is a hypothesis to test, not an established cause.
Sun visibility, caster availability and light selection remain other possibilities.

Both AO PSOs must be ready and observed before reporting application. Optional
PSO creation failure leaves the confirmed default available. No extra ray count,
GPU readback, resource allocation per frame or render-thread wait is added.

## Validation

Instruction tests compare the combined shaders against R139 outside the single
AO load and verify unchanged correction operands. Policy tests cover readiness,
RR preservation, authored clipping, filter choices and both-pass observation.
Windows CI disassembles and creates both new shaders on WARP and exercises
producer/composition/history routing. In-game motion validation remains required.
