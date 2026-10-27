# R186: identify the remaining noisy reflected-light component

R184 helped slightly in the elevator but not on the painting. The user confirmed
that the first R185 test used SPEC4 and showed no visible improvement; its log
upload belonged to a different retest. No repeat of that experiment is needed.
R186 disables the R185 phase correction and retains R184 as its full baseline.

The earlier R183 split separated emission from all remaining shading. It showed
that the painting's noisy reflected lighting was in the remaining-shading branch,
but did not distinguish local specular highlights from diffuse/environment
lighting. This build makes that narrower attribution. It is a diagnostic,
not a proposed quality improvement or a performance optimization.

## Run

Close Control, install the inner ZIP beside Control_DX12.exe, and keep the
existing NVIDIA runtime files. Run **Start-Spec-Local-Test.cmd**.

| Option | Reflected lighting retained | F10 label |
|---|---|---|
| 1 | Full R184 baseline | SPEC: HIT VIEW |
| 2 | Everything except secondary point/spot specular highlights | REFLECTION: OTHER |
| 3 | Only secondary point/spot specular highlights | REFLECTION: LOCAL SPEC |

Start with option 2. Select NATIVE BLEND, RR on, FG off, **SPEC4**. Check the
stationary painting and elevator, then SAVE LOG. Close the game and repeat
with option 3. **SPEC resets to Native on restart: select SPEC4 each time.**
Report which option retains the splatter, or whether both do. No exact camera
alignment or full multi-part capture is required initially. Screenshots help
if the visual result is ambiguous. Do not compare FPS or treat darkness as a fix.

The split affects reflected lighting, so primary lighting remains visible.
Option 3 does not mean every visible light in the image has been isolated.
To return to normal reflections, close the game and choose option 1 or launch
normally. The launcher uses a new process-local environment variable; older
emission/shading launcher settings do not select these modes.

## Implementation and interpretation

The native point loop accumulates specular radiance in r32.xyz. After the point
loop, r34.xyz starts with r32.xyz and accumulates spotlight specular radiance.
At instruction 942, native resolve combines this with environment specular.
Option 3 saves only r34.xyz. Option 2 reconstructs the other component directly:
environment specular + diffuse lighting times textured albedo + emission.
It does not subtract nearly equal radiances to obtain the remainder.
Environment misses belong to option 2 and are zero in option 3.

Both shaders still compute full native radiance and use that total for the
per-hit brightness-cap factor. Only the numerator/uncapped contribution changes.
R184 local-light viewing direction and R172 load scheduling are present in all
modes; R176 retry reuse remains. Native resource bindings, ray counts, light
loops, shadow lookups, material textures and RR guides remain. There is one
extra temporary in isolation modes. Later filtering/RR is nonlinear, so the
two displayed images need not sum to the displayed baseline.

If local-only retains the painting splatter and other-only removes it, the next
candidate is secondary-highlight integration/regularization. If other-only
retains it, broad reflected shading/environment sampling remains the priority.
If both retain it, neither branch can be treated as the sole cause. This test
does not prove a particular denoising algorithm or roughness change will help.

Path regularization can trade highlight fidelity for reduced sampling variance
([PBRT, Path Regularization](https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer#PathRegularization)).
No roughness broadening is applied here: first determine whether that branch
actually carries the painting artifact before accepting that tradeoff.

## Validation

Pinned-source checks reject unknown shader bytecode. An instruction audit traces
the local-specular accumulator and reverses every isolation edit to the exact
R184 baseline. A decoded-token interpreter checks 10000 independent RGB cases,
including packed xyw channels and the original-total cap rule. WARP fixtures
compare emitted component arithmetic with independent closed-form references
across sample counts, caps, partial dispatches and sentinel termination.
Full-shader PSO validation, startup routing/fallback tests, and actual Windows
CMD selection tests are separate build gates. These are not full-scene DXR/RR
quality tests.

`SPEC_LOCAL_COMPONENT build=R186 requested=other|local applied=1 native_fallback=0`
confirms creation used the chosen shader. The F10 label alone identifies the
requested mode. Unknown shaders and failed creation keep native behavior.
Experimental branch only; no production branch or release is changed.
