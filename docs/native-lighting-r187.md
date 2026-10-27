# R187: reflected diffuse versus environment specular

R186 option 2 retained the reported splatter. Option 3 was too dim to judge,
so it is inconclusive. The uploaded Lighting logs verify RR-on/effective-SPEC4
intervals but do not carry the isolation-creation messages from the probe log.
R187 fixes that reporting gap and narrows the remaining shading branch.

This is a diagnostic, not a performance or quality fix. Normal full mode keeps
R184 rendering. The R185 precision experiment remains disabled.

## Test

Close Control and install the inner ZIP beside Control_DX12.exe, preserving
the existing NVIDIA runtime files. Run **Start-Spec-Indirect-Test.cmd**.

| Option | Reflected contribution retained | F10 label |
|---|---|---|
| 1 | Full R184 baseline | SPEC: HIT VIEW |
| 2 | Diffuse lighting at the reflected hit, including local and environment diffuse | REFLECTION: DIFFUSE |
| 3 | Environment specular lighting and environment misses, boosted 8x | ENV SPEC x8 |

Inspect option 2, then option 3. Each launch: select NATIVE BLEND, RR on, FG off,
and **SPEC4**. SPEC resets on restart; select it again each time. Check stationary
painting and elevator splatter and SAVE LOG for each mode. Report which mode
retains the splatter, both, neither, or inconclusive. Screenshots help if unclear;
no full multi-part capture or exact camera matching is required initially.

Darkness is intentional in isolated modes. The option 3 boost is for visibility,
not a material/lighting correction. If the signal remains too dim or clips badly,
report that instead of calling it clean. The boost can affect RR's response, so
this is not a controlled comparison of final-image noise at native brightness.
Both modes exclude reflected emission and secondary point/spot specular. Primary
scene lighting remains visible. Option 1 or normal launch restores full output.

## Implementation

Diffuse isolation saves r14.xywx * r16.xyz after the native diffuse sum and
textured albedo are prepared, before environment/local specular are added.
Environment isolation saves r21.xywx * r29.xyz at the native environment-specular
composition site. For environment misses, it retains r29; diffuse returns zero.
The complete native total is still calculated for the per-hit brightness cap.
Only the contribution numerator/uncapped selection is replaced. Environment's
8x gain is applied after that cap, before accumulation and averaging.

R184's hit-view correction, R172 load scheduling, and R176 retry reuse remain.
There are no new rays, texture reads, roughness changes or filters. Isolation
adds one temporary and a few arithmetic instructions; do not use diagnostic
timings as evidence of a production performance gain.

The Lighting log now includes `spec_indirect_status` at five-second heartbeats
and SAVE LOG, with requested mode, diagnostic gain, successful direct/library
creation counts, and failures. Nonzero creation counts establish that the
matching shader was successfully created; they do not prove every dispatch or
unknown shader variant uses it. Zero creations or failures require inspection
before interpreting an isolation result. Detailed probe logs additionally keep
`SPEC_INDIRECT_COMPONENT applied=1 native_fallback=0` for each matched creation.

## Validation

The transform rejects unknown native hashes. Reversing its component and gain
edits recovers R184 exactly. Source provenance and a decoded-token interpreter
check independent RGB channels in 10000 cases. GPU fixtures execute both hit
and miss paths, compare independent algebraic references, and exercise native
sample counts, cap settings, partial groups and sentinel termination. The 8x
gain is tested independently of the cap factor. Full-shader PSO validation,
runtime routing/failure tests and actual Windows CMD menu tests are separate
build gates. These tests do not establish in-game RR image quality.

Experimental branch only. No production branch or release changes.
