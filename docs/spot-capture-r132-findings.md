# R132: repeated near-light geometry blocks the captured rays

Evidence: `Lighting-R132-20261023-200802-31528-0.log`, capture generation 91303,
frame 91303, native image 2560 × 1440. The strict decoder accepted provenance,
record counts, normalized ray directions, barycentrics, pixel mapping and all
completion markers. There are 3,632 valid rays, 3,631 hits and one miss. All
12,690 shader-table entries were accounted for: 6,345 translated and 6,345 null.
No unknown identifiers or invalid records were reported. Export completed.

## Dominant blockers

| Selected light | Valid rays | Dominant instance | Hits on that instance | Endpoint Y | Blocker hit Y range |
|---|---:|---:|---:|---:|---:|
| 69 | 1,243 | 3107 | 1,107 | 31.4375 | 31.1963–31.3010 |
| 71 | 507 | 3257 | 387 | 31.4375 | 31.1963–31.2756 |
| 72 | 1,013 | 3258 | 789 | 31.4375 | 31.1963–31.3010 |
| 73 | 865 | 3259 | 634 | 31.4375 | 31.1963–31.3073 |

These four instances account for 2,917 of 3,631 hits (80.3%). Coordinates are
native world units; no conversion to meters is assumed. IDs are local to this
TLAS capture and must not be hardcoded into a runtime fix.

The dominant surfaces are roughly 0.13–0.24 world units below the light
endpoints. Their ray-distance gaps vary much more widely: up to 2.35 units for
light 73. Grazing incidence makes a fixed endpoint-distance cutoff unsuitable:
it can retain the same surface at one angle and skip unrelated blockers at
another. The measured hits are well beyond a tiny floating-point endpoint
error. This supports the earlier observation that a broad END GUARD restores
brightness but damages shadow shapes; it is not a replacement for that visual
evidence.

## Repeated mesh evidence

For each sufficiently sampled triangle, solve `barycentrics × vertices = hit
positions`. Reject rank-deficient fits, condition number >= 100 and residuals
>= 0.00001. Compare reconstructed triangles with matching geometry/primitive
indices between dominant instances:

- Lights 69 and 72: 21 common triangles match after translation approximately
  (0, 0, 8), maximum residual 0.0000098.
- Lights 72 and 73: 25 common triangles match after translation approximately
  (4, 0, 0), maximum residual 0.00000292.

This establishes repeated geometric structure, not just similar hit distances.
The spatial association supports fixture self-occlusion as the leading
explanation. Asset names, emitting-surface ownership and original shadow-map
caster exclusions are not present in this capture, so a specific fixture asset
or intended exclusion rule is not yet established.

## Correction constraints

Preserve real geometry shadows. Do not discard all hits near a light, ignore
backfaces globally, move every light by a constant world-space offset, or ignore
these four transient instance IDs. A general correction needs a verified
light-to-fixture/emitting-surface relationship or the original per-light caster
selection rule. The current evidence narrows the investigation to that mapping;
it does not justify changing RR, adding ray samples or modifying material guides.

The complete capture is sufficient for this analysis. No repeat of the same
capture is needed. This commit changes analysis only, not game rendering.

Reproduce with Python and NumPy:

```
python tools/analyze-spot-hit-blockers.py PATH_TO_R132_LOG
```

The tool always runs the strict capture decoder first. Its output includes
per-light endpoints, dominant geometry bounds and triangle-translation fits.
