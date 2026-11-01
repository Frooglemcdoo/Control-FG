# GI speckles: source audit and correction decision, 2026-11-01

## Decision

Continue targeted reverse engineering of the GI producer, hit shading and RR composition. Do not ship the constant-attenuation diagnostic or raise a denominator floor on the strength of screenshots. Do not require a full-game decompile. No rendering change is made by this research commit.

## User evidence

- Destroy desk lamp / disable RT indirect: both affected objects improve.
- R267 no point GI: no improvement; no spot GI: hair clears and plant largely clears.
- R268 manual PCF: no improvement.
- R269 final PCF/VSM visibility set to 1: no improvement. Logs confirm shader creation and recorded binds, raw GI bypass during RR-active interval; not per-pixel GPU execution evidence.
- R270 white projection cookie: no improvement. Flat attenuation: hair clears, plant dots smaller. No secondary Fresnel reduction: as bad as white cookie.
- GI16 made dots smaller, without eliminating them.
- Both-filter mode previously removed speckles but ghosted. Clamp-only and spatial-only each traded different artifacts. These results do not justify restarting that filter-tuning loop.

## New reverse-engineering finding

`tools/audit-gi-attenuation.py` reads the supplied loose native shader archive, extracts DXBC containers, and checks arithmetic operands plus the provenance of coefficients. Output: `research/gi-source-r267/attenuation-crosscheck.json`.

Across 8 direct-light shader variants, 4 reflection-hit variants and all 8 GI-hit variants, the same spotlight scalar attenuation is present:

```
D = dot(lightPosition - shadedPosition, lightPosition - shadedPosition)
A = max(1 / (pi * max(c76 * D + c72*c72, 0.25)) - c68, 0)
```

Here c68, c72 and c76 mean floats from bytes 68, 72 and 76 of the 320-byte spotlight record. The audit proves the coefficients are loaded from `g_sbDeferredLightSpotData` and their register components are not overwritten before use. Their authored names, units and actual lamp values are NOT established. The position subtraction is separately visible in the source assembly; matching coordinate-space meaning at runtime is still an audit task.

Variant-0 GI: coefficient load DWORD 3876; distance calculation 4633–4641; attenuation 4648–4691; range fade application 4698; light color application 4705; projection texture multiplication 4817; near-plane/shadow multiplication 4839–4846; diffuse term and accumulation 4915–4989.

The denominator already has a 0.25 floor. For finite inputs its reciprocal portion cannot diverge at zero distance: it is at most 4/pi. If c68 is nonnegative, A is also bounded by 4/pi. This does NOT bound total radiance: authored RGB/intensity, material response and sampling still matter. It also does not establish that the record is valid or the coordinate spaces agree.

R270 flat falloff changes A to 1/pi; it changes both distance dependence and energy. Improvement is therefore not proof of an invalid native formula or proof that variance alone was reduced. A matching native formula in three paths substantially weakens the GI-only-arithmetic-error theory, while leaving input/precision/weight/composition errors possible.

## Primary-source research

[Ray Tracing in Control, NVIDIA/Remedy, sections 46.4 and 46.6](https://developer.download.nvidia.com/ray-tracing-gems/rtg2-chapter46-preprint.pdf): the documented design traces cosine-distributed near-field GI rays and shades their hits separately. It suppresses specular hit shading and uses baked GI for fallback/second-bounce approximation. Its denoising includes firefly clamping before temporal and spatial processing. This describes the published design, not proof of every present runtime detail.

[NVIDIA RR integration checklist](https://github.com/NVIDIA-RTX/Streamline/blob/main/plugins/streamline-agent-skills/skills/integrate-dlss-rr/references/integration-checklist.md) and [RR programming guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md): RR expects current noisy HDR color with coherent guides; bypass existing denoising unless justified. The source's historical reliance on firefly handling is not permission to stack native temporal filtering ahead of RR.

[PBRT path sampling](https://www.pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Path_Tracing): concentrating samples on important indirect contributors requires corresponding sampling weights. Merely directing GI rays at analytic lights does not sample the brightly lit secondary surfaces that contribute the bounce.

[ReSTIR GI integration](https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md): reuse requires secondary position/orientation, sampling PDF and secondary radiance, plus target-density and visibility/Jacobian validation. It is a possible architectural solution to measured variance, not a drop-in switch or guaranteed fix. [Research paper](https://research.nvidia.com/publication/2021-06_restir-gi-path-resampling-real-time-path-tracing).

## Required audit before another correction

1. Follow the native ray generator through direction mapping, origin bias, phase sequence and ray index to the hit textures. Recover the actual probability distribution; verify material/position/normal correspond to the same sample. Preserve the accepted R264 sequence while checking its correctness.
2. Follow light-table producer fields, layout, view-space transform and clustered indices into the GI consumer. Compare the SAME light record between direct and indirect shading, not merely two similar code blocks. Audit FP16 hit-position sensitivity and invalid/out-of-range accesses.
3. Trace diffuse energy, albedo/Fresnel modulation, sample weighting and count normalization through raw GI, native blend composition and RR color/guides. A correct arithmetic average is sufficient only for the distribution/throughput that generated those samples. Do not apply NRD's demodulation contract to RR's full-color input.
4. Retain native shadows/cookie/Fresnel/falloff for measurement. Instrument a bounded ROI at the original plant and hair, before per-ray accumulation. Save frame/ray/pixel, hit/miss/material, primary/secondary positions and normals, light index, raw coefficient/color records, distance squared, denominator, attenuation, visibility, projection color, diffuse factor, pre/post material radiance and final per-ray contribution. Include overflow and completion markers; do not confuse command recording with completed GPU writes.
5. Build a same-camera comparison of noisy GI, RR and an accumulated reference with independent samples. Existing captures establish the noisy source but do not provide all per-ray/light operands needed above. Quantify outlier frequency and share of total energy, mean brightness, variance, invalid samples and correlation to the lamp. Match exposure, geometry and lights for comparisons; moving content cannot be blindly averaged.

## Correction gates

- Wrong record/space, nonfinite values, incorrect PDF/count or double modulation: correct that contract first, with regression checks.
- Valid rare high-energy bounce samples: improve the estimator via measured sample allocation, indirect-path importance sampling or carefully validated spatial reuse. Preserve expected illumination; include correct weights and visibility checks. ReSTIR GI is one candidate, not yet the selected implementation.
- Noisy input and reference agree but RR output fails: audit/revise RR composition, guides and temporal correspondence rather than weaken lights.
- Bounded firefly handling is a fallback requiring quantified energy loss and RR validation. It must not silently erase legitimate bright paths or restore the prior ghosting history.

Acceptance: correct mean bounce lighting against reference; original plant AND hair improve; shadows and projection patterns retained; stationary speckles, motion boiling, disocclusion and ghosting checked; performance measured. No per-object or lamp-specific exclusions.
