# R188: environment source isolation

R187's uploaded logs confirm creation of both requested component shaders with
no failures, and include SPEC4/RR-on intervals. Its screenshot shows ENV SPEC x8,
SPEC4 and RR OFF, with grain on elevator walls. This establishes visible noise
in the boosted environment diagnostic without RR; it does not establish that
this is the only source of the normal-brightness RR splatter, or that reflected
diffuse is clean. No visual outcome for the diffuse-only run was supplied.

The native environment-specular path uses either directional illumination
volumes or the environment cubemap. Its incident direction already follows the
primary-to-secondary ray. R184's local-light view correction is not applicable
here. Before changing source sampling, this build isolates those two routes.

## Test

Close Control, install the inner ZIP beside Control_DX12.exe and preserve the
NVIDIA runtime files. Run **Start-Spec-Environment-Test.cmd**.

| Option | Signal | F10 label |
|---|---|---|
| 1 | Full R184 baseline, normal brightness | SPEC: HIT VIEW |
| 2 | Environment specular from directional volumes, boosted 8x | ENV VOLUME x8 |
| 3 | Environment specular from the cubemap, boosted 8x | ENV CUBE x8 |

Each launch: NATIVE BLEND, RR on, FG off, SPEC4 (resets at restart). Check the
stationary painting and elevator, then SAVE LOG. Report which option retains
the splatter. A dark/absent source is inconclusive about its reconstruction
quality, not evidence that it is clean. No exact camera match or full capture
is needed. Both options deliberately omit other reflected lighting. Normal
launch or option 1 restores full R184 output.

This is source attribution, not a proposed quality or performance fix. The 8x
boost affects RR's response and cannot establish normal-brightness severity.
Primary scene lighting is still visible. Prior R187 diffuse/environment routes
remain internally supported for regression tests but are not in this launcher.

## Implementation and checks

Native instruction 367 selects the volume branch; its else at 377 selects the
cubemap. Both sides assign a new per-sample integer source tag in r44.x before
hit/miss shading. A movc gates R187's saved environment contribution in r43,
after hit/miss shading and before the cap numerator. Complete original radiance
remains in r29 for the original-total cap. Both modes retain R187's post-cap 8x
gain, R184 local view correction, R172 load scheduling and R176 retry reuse.
The R185 phase experiment remains disabled.

There are no added rays or texture reads and no mip, roughness, BRDF, cap or
history changes. The diagnostic adds one temporary and source selection ALU
relative to R187 environment mode. It does not skip source computation.

The transform accepts only the pinned native SHA. Removing its source tags,
gate and temporary change recovers R187 environment bytecode exactly. Audits
check source provenance, per-sample assignment on both routes and disjoint,
exhaustive selection. GPU fixtures alternate routes across pixels, execute the
production if/else and gate, and compare an independent direct selection for
both hit and miss paths. Native sample loops, cap settings, partial groups and
sentinel termination remain covered. Full-shader PSO checks, runtime ownership
and failure tests, and actual Windows CMD selection tests are separate gates.
These checks do not establish in-game image quality.

Lighting logs report requested=volume/cube, gain=8 and creation/failure counters.
Successful creation does not establish replacement of every possible shader
variant or dispatch. Experimental branch only; no production or release edits.
