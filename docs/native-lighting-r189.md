# R189: roughness-dependent volume specular filtering

## Why this change

The user confirmed that R188's noisy painting screenshot was option 2
(directional-volume specular) with RR ON. Both uploaded runs successfully
created their requested shaders with zero reported failures. Option 3 was too
dark to assess and remains inconclusive. The isolation used an 8x gain, so it
establishes that the volume contribution can carry the artifact, not that it
is the sole cause or that normal-brightness severity is identical.

Native shader inspection shows two volume lighting evaluations: one along the
reflected direction and another along the secondary surface normal. Their
six-lobe directional interpolation does not depend on roughness. The cubemap
route instead chooses its mip from roughness. Roughness already affects the
later environment BRDF lookup in both routes; this change targets only the
volume lighting's angular evaluation, not that BRDF.

## Test

Close Control and install the inner ZIP beside Control_DX12.exe, retaining the
NVIDIA runtime files. Run **Start-Spec-Volume-Test.cmd**.

| Option | Behavior | F10 label |
|---|---|---|
| 1 | Roughness-dependent volume filtering | VOLUME: ROUGHNESS |
| 2 | Full R184 baseline | VOLUME: BASELINE |

Both options keep full lighting at normal brightness. Use NATIVE BLEND, RR ON,
FG OFF, and SPEC4 on every launch (SPEC resets at restart). Check stationary
painting/elevator splatter, reflected lighting detail, brightness changes and
FPS. SAVE LOG for each option. Exact camera placement and a full capture are
not required initially. Normal launch enables the filter; option 2 disables
it for that launch. No game configuration file is modified.

## Approximation and scope

For secondary hits in the native volume branch:

    t = saturate(1 - secondarySmoothness)^2
    filteredVolumeSpec = (1-t)*reflectionDirectionVolume + t*normalDirectionVolume

The two lighting values are already evaluated, normalized across volumes and
scaled alike. This inexpensive blend approximates broader angular response
on rough secondary materials. It is not an exact GGX prefilter and does not
establish an unbiased estimator. It may soften directional lighting, shift
its brightness/color or fail to address noise from varying hit points,
materials, volume boundaries or RR reconstruction. The volume representation
is already a coarse directional lighting representation; this change does
not recover missing source detail.

The blend leaves perfect-smooth secondary hits and environment misses alone.
The cubemap route, secondary point/spot lighting, diffuse term, emission and
BRDF LUT are unchanged. It uses only secondary roughness, with no primary
roughness floor or modification of ray sampling. Both input values being
equal gives unchanged output; otherwise each RGB result lies between them.
That flat-field property is not a claim of global energy conservation.

The native brightness cap runs after the modified complete lighting sum, as
usual. This differs from the earlier diagnostic component isolation, which
retained the original total solely for source attribution. There is no 8x gain.
R184 hit-view correction, R172 load scheduling and R176 retry reuse remain;
R185 phase precision and R186-R188 isolation flags are disabled.

## Implementation and validation

Five ALU instructions are inserted after native instruction 376, inside the
volume route and before its else. r22.y holds secondary smoothness; r6.w still
holds the native miss predicate. r27/r29 contain normalized, scaled normal-
and reflection-direction lighting. r7.w and r25.xyz are dead scratch at this
site and overwritten before their next native use. No temporaries, rays or
texture reads are added. Actual GPU performance still needs measurement.

The transform rejects unknown source hashes. Removing the five instructions
recovers R184 exactly. Source/liveness audits and an independent decoder test
10000 randomized RGB cases, convex bounds and smooth/miss/flat-field behavior.
GPU fixtures compare the emitted block against an independently simplified
algebraic reference at smoothness 0, 0.5 and 1, for hits and misses, across
native sample counts, cap settings, partial groups and sentinel termination.
Full-shader PSO validation, direct/library routing and failure ownership tests,
and actual Windows CMD ON/OFF/cancellation tests are separate build gates.
These checks do not prove in-game image quality or a performance gain.

Lighting logs record spec_volume_status with enabled, gain=1 and creation /
failure counters. Creation counters do not prove coverage of every unknown
shader variant or dispatch.

## Research context

PBRT discusses how concentrated indirect radiance can produce high variance
and how regularization trades bias for reduced variance:
https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer

OPSR likewise describes roughness-based regularization as a bias/variance
tradeoff: https://diglib.eg.org/items/506f26f8-09c7-4973-942e-a8f97da1f405

These sources motivate testing controlled angular broadening; neither
validates this game's volume representation or the particular blend above.
The implementation is an experiment derived from the inspected shader.

Experimental branch only. No production branch or release edits.
