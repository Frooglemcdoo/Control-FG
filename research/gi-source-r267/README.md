# Native RT diffuse GI source audit

Status: investigation, not a rendering fix. The optional R267 source probe is
implemented; normal launch preserves the baseline. See `docs/native-lighting-r267.md`.

User evidence: removing the desk lamp removes speckles on the affected plant
and Jesse's hair. Disabling RT indirect diffuse lighting also removes both.
Increasing GI samples to 16 reduces speck size without resolving the issue.
These observations establish a dependency, not the exact defective operation.
The required correction must apply to the shared lighting path, without object,
material, light-ID, or room exceptions.

## Evidence

Source: `rt_deferred_diffusegi.obj` in
`Control-RR-Loose-Shaders-20260912-212234-687d93d6.zip`.
Nine embedded DXBC containers were decoded with
`research/native-integration-audit/tools/decode_dxbc.py`.
`manifest.json` records full container identities. The compact assembly listings
use SHEX DWORD offsets, not file byte offsets. Variants 0–7 shade GI;
variant 8 resets a compaction counter. Bindings and reflected constants are
retained separately. The decoder is an inspection aid, not a shader validator;
integer immediates may be printed as floating-point bit interpretations.

## Traced path (variant 0)

| SHEX offset | Operation |
|---|---|
| 1002–1058 | Load secondary material, position/UV and normal/UV arrays |
| 1147–2257 | Evaluate baked illumination volumes |
| 2317–2360 | Load secondary base-color multiplier and sample base texture |
| 2398–2596 | Evaluate secondary material's diffuse energy factor |
| 2674–2740 | Select point-light list from secondary position's light cell |
| 2833–2883 | Load point-light record, including byte-144 scalar |
| 2893–3020 | Calculate point-light direction, distance, range and attenuation |
| 4664–4846 | Spot-light attenuation, projection and visibility |
| 5006–5018 | Combine local lights with baked illumination; multiply by secondary albedo |
| 5036 | Accumulate ray contribution with primary diffuse energy weight |
| 5150–5157 | Divide accumulated lighting by configured ray count |
| 5191–5205 | Store hit information, color and weight |

The point-light denominator is `pi * max(distanceSquared + q*q, 0.0001)`,
where `q` is loaded from point-light record byte 144. The precise engine field
name and the desk lamp's runtime value are not established. Range falloff,
subtraction, light color, visibility and diffuse BRDF factors also affect the
contribution. This is not an unbounded inverse-square singularity.

All eight variants contain this point-light denominator floor and a separate
spot-light denominator floor of 0.25. This difference alone is NOT proof of a
bug: the light models and parameters differ. Raising the point-light floor
would alter legitimate illumination and is not an evidence-backed fix.

The per-ray color accumulation and final division both use the configured
sample count in variant 0. No missing final ray-count normalization was found
in that traced path. This does not validate the ray generator's sampling PDF.

## What this establishes

Local lights contribute to RT indirect lighting at secondary hits. Therefore a
lamp can trigger noisy indirect lighting on multiple receiving materials even
though ordinary direct lighting is a separate pass. The shader has a shared
location suitable for investigating this across the game.

It does not establish whether the lamp is a point or spot light, whether its
largest samples are valid, or whether source variance, shadow visibility,
sampling weights, or RR guide interpretation is the remaining cause.

## Next implementation boundary

Use the existing captures first; their combined GI image cannot by itself
separate baked, point and spot contributions. If further instrumentation is
necessary, observe contributions before the final sum in the exact native
variants, retaining direct light, ray count, history policy and RR settings.
Any future capture must distinguish contribution magnitude from a count of
bright pixels. Do not ship an attenuation change, global clamp, temporal
history increase, or material exclusion as a proven fix from this audit.

The independent R266 foliage admission expansion does not explain or resolve
the original affected plant, whose shader was already admitted.

## R267 user result, November 1

The user reports no visible improvement with option 2 (no point-light GI).
Option 3 (no spot-light GI) removes visible hair speckles and most plant
speckles; minor plant speckles remain. Supplied video:
`20261101-1424-31.2275511.mp4`, 31.07 seconds, 2986x2138 at 30 fps.
Frames inspected at 1, 7, 22 and 29 seconds show the affected original plant
and Jesse's hair, but do not show the diagnostic mode or binding log. Treat
mode attribution as user-reported, not independently verified from the video.

This prioritizes the spotlight contribution. It does not establish that all
remaining plant noise has the same origin, or prove an attenuation defect.
In particular, the previous point-light denominator candidate is deprioritized.

The traced secondary-hit spotlight visibility uses the native PCF/VSM shadow
atlases (variant 0 offsets 4182, 4325, 4372), followed by visibility combination
at 4494–4549 and application at 4846. The PCF branch performs a depth texture
sample followed by comparison at 4338. The traced GI shader does not read the
primary-surface RT spotlight visibility result. Consequently, primary spotlight
shadow improvements cannot be assumed to fix secondary-hit GI visibility.
Which atlas branch is active for this lamp remains unknown. Next investigation
must distinguish visibility/projection sampling from light falloff and BRDF
weighting while restoring the full spot-light contribution. Removing that
contribution is an isolation result, not the intended final correction.
