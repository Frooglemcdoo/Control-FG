# R202 capture: projection paths and authored clipping

Source: Shadow-R202-20261028-141148-10732-0, generation/frame 228454.
Native Blend, MAP GUARD OFF, RR ON. The existing decoder accepted 4096 records.

## Measured result

Restrict to PCF-only lights, supported native center == 1, raw spot visibility
== 0, and native filtered map minus filtered RT visibility > 0.25. Light 23
has 279 such samples; all 279 reference rays hit geometry. Other selected
lights contribute five qualifying samples (10: one, 25: four).

The new offline analyzer independently solves the captured row-vector projection
for its finite apex (homogeneous x=y=w=0). For light 23 it agrees with the
captured cone apex within 8.84e-7 engine units. The apex-to-emitter separation is
38.443729 units. This is a shadow-projection source versus RT-target difference,
not evidence that two engine lights occupy the same place.

| Measurement, light 23 | Minimum | Median | Maximum |
| --- | ---: | ---: | ---: |
| Reference versus projection direction, degrees | 6.87565 | 15.56968 | 42.68086 |
| Path separation at reference-hit cone-axis depth, engine units | 0.0000482 | 0.682684 | 2.886292 |
| Hit margin past authored near plane, engine units | 2.186252 | 4.539584 | 12.201533 |
| Reference hit distance, engine units | 0.0003163 | 2.782168 | 8.882606 |

No qualifying hit lies on the excluded side of the authored near plane.
268 reference hits report kind 254 and 11 report kind 255. This alone does not
establish the raster caster culling policy or justify rejecting either face.

## Interpretation and next measurement

This supports the user's observation of different shadow angles. It does not
show whether a ray along the native projection path would miss these blockers.
The path construction starts at the biased RT receiver; native receiver bias
and atlas depth comparison remain different. A plane-side check is not an
atlas caster-membership test. Capture-local light/instance IDs are not assets.
The exact close-up wall cutoff is still outside the captured wide-view evidence.

R162 must remain documented as a failed visual experiment. Its reported
false-edge light had an apex/emitter separation of 0.652704 units; the present
light has a much larger separation. That failure does not establish equivalence
for this light, but neither does the current geometry justify restoring that
rendering mode as a fix.

Next diagnostic: capture a projection-apex reference ray beside the accepted
emitter reference ray using the same TLAS, receiver, material handler, instance
mask, frame and selected light, with independently computed authored-plane
clipping for each path. Record both directions, limits and hit identities.
Preserve Native Blend rendering, RR, SPEC Native, MAP GUARD and camera state.
First verify the projection target from the actual matrix; do not silently
assume the cone field is its apex for every light. Clear projection rays would
support a path mismatch; continued hits would keep caster/bias discrepancies
open. No hard-coded room/light exceptions or speculative brightness changes.

## Delivered scope and validation

Adds only an offline analyzer and analytical geometry regression fixtures.
Tests cover a known perspective apex, translated coordinates, known angular
separation and equal-depth intersection, clipping-side sign, misses, singular
and nonfinite matrices. Existing missing-light analyzer tests also pass.
No shader/runtime change, Windows build, new downloadable DLL, or claim of
in-game validation is part of this analysis commit.
