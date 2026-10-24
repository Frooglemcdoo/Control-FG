# R134: authored spotlight plane comparison

R133 captured 4,096 rays across 16 selected lights. Its context reconstructs the
native ray endpoints within 0.000055 world units (including the existing endpoint
epsilon). Applying the candidate plane places 110 of 2,149 captured hits on its
light side, with 2,039 on its receiver side. The computed emitter-to-plane offset
varies with authored light data: 0.1, 0.12, 0.3 and 0.5 in this capture. These
counts support a live comparison; they do not establish intended asset semantics
or prove that every near-light surface should be excluded.

## Live comparison

Install with Control closed. Keep native RT Contact Shadows on, samples 1,
FG and glass enhancements off. Compare BOTH and AUTHORED CLIP in F10. BOTH keeps
the established R97 centered shadows; AUTHORED CLIP changes only the spotlight
ray end. Allow the shadow history to settle after switching. No restart needed.
Check the affected tables/floors, Jesse and objects close to lights, other rooms,
and fixtures viewed at grazing angles. Send screenshots/video and Lighting-R134
from %LOCALAPPDATA%/Control-FG/Diagnostics. SAVE LOG is optional. There is no
required 70-second capture wait for the visual comparison.

## General rule under test

Read each selected spotlight's native cone position, direction and fNearDistance.
Transform the position and direction using the same inverse-view rotation and
translation used by the native emitter. Normalize the direction and construct
its authored axial plane. Intersect the receiver-to-emitter segment with that
plane only when the receiver is in front and the emitter is behind it. Preserve
the existing small endpoint epsilon. Invalid light parameters, noncrossing rays,
and receivers inside/on the plane retain the original full segment.

This is a plane intersection, not distance-to-cone minus near distance. The
receiver bias and direction are unchanged. Four centered rays, alpha testing,
point shadows, blocker averaging, temporal/spatial filtering, GI filtering and
RR eligibility remain unchanged. No hardcoded mesh/primitive IDs, room positions,
global cutoff distances or additional rays are used. The rule applies to every
selected spotlight using this replacement path, not all point lights or every
shadow system in the game.

The new library is optional. If its pipeline group fails to link, the comparison
button is disabled and the established baseline remains available. A seventh
immutable shader record holds the variant; state-object storage is reserved
before associating subobjects so adding the export cannot invalidate pointers.

## Validation and limits

Signed DXIL validation and exact reversal of all edits to hash-locked R97;
independent numerical tests for oblique/rotated planes, near-receiver contacts,
invalid parameters and noncrossing segments; mode/readiness, record bounds,
normal composition and RR-policy tests. The strict 132-file baseline audit and
17 packaged lighting/glass sidecars remain unchanged. Visual correctness and
performance on the live driver still need this comparison. Do not describe the
candidate as a proven universal fix until it passes across affected scenes.
