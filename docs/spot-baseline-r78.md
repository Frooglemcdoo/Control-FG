# R78: consistent R71 spotlight sampling

R71 is the user-confirmed last good build. R77 still showed blobs, pop-in and
noise; restoring native map occlusion looked like ordinary shadows and is not
a replacement-shadow fix.

## Changes

- The single-ray sidecar is byte-identical to R71 (SHA-256
  `f0127f6267359eccf61fda126ecc9ff733d0b9090f6e9d16a88f65840e7d3ab8`).
- The alternate/configurable native shader now uses the same four symmetric
  spotlight disk samples, frame-index seed, stable radius, closest-hit traversal
  and light-range limit. Its point-light loop, seed, count, normalization and
  shared hit/miss handlers remain native. This corrects the policy mismatch
  introduced when support for that library was added in R75.
- Both paths retain R71 bounded temporal history and spatial filtering.
- R73's sampled-emitter endpoint rewrite is removed. The native light near-plane
  exclusion is retained, as in R71; this does not claim to fix the original
  incomplete character shadow near a light.
- R72 caster-expansion hooks are not installed or required for readiness.
- The overlay offers NATIVE SHADOWS and RT SPOTLIGHT. RT SPOTLIGHT uses the
  replacement composition; it does not restore native map occlusion.
- R69 asynchronous logging remains in place.

## Test in one session

Close the game only to install all files from this package beside Control_DX12.exe.
Enable RT Contact Shadows, load the affected area, open F10 and select RT SPOTLIGHT.
Compare against NATIVE SHADOWS live, without restarting. First check for the later
blob/weapon/noise regressions, then the original shadow pop-in and close-light
character coverage. Compare RR off/on at the same position, including standing
still. Save one R78 log and describe each remaining symptom separately.

This is an experimental build, not a confirmed visual fix. It still replaces only
one selected spotlight per pixel. CPU submission logs do not prove GPU image
correctness. The original R71 coverage/flicker issues may remain. The alternate
shader now traces four spotlight rays regardless of its native sample-count option;
point-light cost still follows the native option.

## Validation

DXIL assembly and validation pass for both captured native libraries. Tests check
packaged hashes, unchanged shared handlers and point-light instructions, the
spotlight-only loop count and normalization, and the exact R71 single-ray hash.
Windows CI additionally builds the DLL and checks compute PSOs and live selection
with WARP. These checks do not substitute for the in-game comparison.
