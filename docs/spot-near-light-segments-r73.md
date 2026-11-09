# R73: spotlight segments near the light

R72 logs confirmed that caster expansion was active, but the reported incomplete
character shadow near lights remained. R73 isolates the ray endpoint calculation.

The native contact-shadow shader subtracts `fNearDistance` from its light distance.
The previous replacement reused that shortened distance for all four disk samples.
R73 measures each segment from the native biased ray origin to that sample's native
light position, normalizes that vector, and stops one part in 2^20 before the sample.
This removes the contact-distance cutoff without tracing beyond the sampled light.
Zero-length segments avoid division by zero. Each returned hit is checked against
its own endpoint; missed samples are excluded from nearest-blocker reduction.

The four-sample count, native alpha handlers, point-light branch, R71 closest-hit
traversal and fixed emitter radius remain. All five compute sidecars are identical
to R72. R71 filtering, R72 caster coverage, and nonblocking diagnostic logging remain.

## Validation

- Signed DXIL passes the DXIL validator.
- Segment tests execute emitted instructions for randomized endpoints, zero length,
  near-light blockers, beyond-light exclusion, and all hit/miss combinations.
- Captured shader tests preserve native hit/miss handlers and the point branch.
- Filter and clean-baseline checks pass; Windows CI checks packaged shader loading.

These checks do not establish visual correctness in Control. Geometry near the
sampled light that was previously excluded can now occlude it. The native sampled
light position is retained; its relationship to visible fixture geometry is not
changed. Hallway pop-in and wall flicker remain separate unconfirmed issues.

## In-game check

Close the game and install every file in the package beside `Control_DX12.exe`.
Enable native RT Contact Shadows. Use F10 to compare NATIVE SHADOWS and RT SPOTLIGHT
REPLACEMENT without restarting. First repeat the close/far test near pillar 14,
then inspect the hallway and wall. Preserve the same RR setting for each comparison.
Use SAVE LOG and send the `Lighting-R73` log with a short video of any remaining issue.

This remains a replacement for one selected spotlight per pixel, not every shadow.
