# R180 offline spatial reuse feasibility

The supplied SPEC4 painting and elevator pairs contain large fluctuations in
captured specular lighting before RR. Native resolve adds shaded samples and
divides by the processed count (`research/spec-performance/spec-resolve.asm`,
end of shader). This audit has not established an incorrect estimator weight.
The current clamp already suppresses bright contributions, so another clamp
increase is not justified by these captures alone.

`tools/analyze-rr-material-reuse.py` probes a different operation: symmetric
exchange of RGB energy between adjacent pixels, with weights based on normal,
roughness, specular albedo and depth. It does not use radiance to decide weights,
accumulate history, change sample counts, or invoke RR. Every pair exchange
adds exactly what it subtracts. The fixed weight budget keeps all resulting
weights nonnegative. Crop boundaries have no exchange with their exterior.

This is an offline feasibility probe, not a replacement shader. It does not
repeat the full native temporal/spatial filter from R103, the stronger clamp
from R104, or the temporal blend from R111.

## Measured result

| Crop | Original frame-difference RMS / mean | Radius 1 | Reduction | Radius 2 |
|---|---:|---:|---:|---:|
| Painting | 1.20168 | 0.49066 | 59.2% | 0.41350 |
| Elevator | 3.36138 | 1.26031 | 62.5% | 0.96698 |

These revised values include the center pixel in the weight budget. The first
probe used only the neighbor count, which could erase an isolated center on
a flat guide field and spread its energy into a ring. That implementation
issue is corrected; the earlier 61.6%/63.8% figures are superseded.

RGB sums are preserved to floating-point precision (asserted for both frames
and both radii). These are **not pure noise measurements**: the two frames
have different jitter, and no correspondence correction is applied. Crops
were selected from mostly flat surfaces. No SPEC16 ground truth is available.

## Decision

Do not ship this probe as a quality fix. Primary-surface guides cannot identify
all reflected edges within a single surface. Energy conservation prevents
global darkening but does not prevent redistribution from sharp highlights
into their surroundings. The larger radius also leaves isolated peaks when
guide rejection prevents exchange; neither metric establishes visual fidelity.
The existing captures are downstream of filtering, so applying this before the
native clamp could behave differently.

A runtime experiment needs a validated binding route for material guides at
the specular pass, a reflection-detail protection strategy, and GPU timing.
The present clamp replacement has depth/motion/history/source bindings but
does not have normal/roughness/albedo SRVs. Merely adding shader registers
would be incorrect. No production or experimental runtime was changed here.

## Reproduce

Requires Python, numpy and scipy, with the verified R180 captures reconstructed
into `painting/<frame>/` and `elevator/<frame>/`.

```sh
python tools/analyze-rr-material-reuse.py /path/to/r180 --output research/rr-material-reuse/r180-results.json
```

The JSON records crop coordinates, metadata hashes, raw measurements and
limitations. Captured binaries are not committed.

## Detail-preservation checks

`python tools/test-rr-material-reuse.py` verifies constant-signal preservation,
RGB conservation, positive results, retained center weight, and rejection at a
depth discontinuity for both radii. It also deliberately tests reflected detail
on a uniform surface. A one-pixel reflected line retains only 33.3% of its peak
at radius 1 and 20% at radius 2. Its energy is spread into neighboring pixels.
This is a concrete counterexample to treating conservation as preserved quality.
It is a synthetic limit, not a measured loss in the supplied painting.

## Primary sample-sequence audit

The native reflection library already forms two coordinates from the fractional
parts of `blueNoise + (frame * count + sampleIndex) * alpha`. The two constants
are present in `research/native-integration-audit/reflection-000.ll` immediately
before the primary GGX mapping (`Frc63` and `Frc64`). The radial coordinate then
gets the native 0.4 scale. R179 changed the rejection fallback, not this primary
sequence. R176's fallback memoization remains unchanged.

`tools/analyze-spec-sample-coverage.py` compares that primary sequence with
one independently jittered sample in each of four quadrants. It uses 131,072
random shifts/frame indices, float32 native arithmetic, and synthetic functions
in sample space. It does NOT model scene geometry, actual blue-noise texels,
GGX mapping, fallback rejection, shading, or RR.

| Synthetic function | Native 4 relative RMSE | Stratified 4 | Native 16 |
|---|---:|---:|---:|
| Small circular contributor | 5.6619 | 5.6349 | 2.7085 |
| Broad circular contributor | 0.9960 | 1.3220 | 0.3475 |
| Vertical strip | 2.0269 | 2.1749 | 1.1921 |
| Horizontal strip | 2.0123 | 2.1747 | 1.2111 |
| Smooth periodic lobe | 0.1435 | 0.2503 | 0.0172 |

Simple stratification is not a consistent improvement even in these controlled
cases, so it is not selected for a runtime build. These results do not rule out
better sampling generally or prove what causes the captured bright values.

## Runtime integration audit

- `rr_live_record.h` produces the owned RR guides immediately before evaluation.
  Its own root signature has the five existing source SRVs and four guide UAVs.
- `rr_clamp_strength.h` replaces only the PSO for the validated earlier native
  temporal dispatch, preserving the native root signature and descriptors.
  The shader bindings provide depth, motion, source and history, not the three
  material guides used by the offline probe. It is not valid to assume those
  later guide resources are ready at this dispatch.
- Filtering the already-composed input color would also affect diffuse lighting;
  subtracting the captured specular buffer requires proving composition weights
  and exposure first. That route is not implemented.

## Native resolve contribution map

The final non-miss shaded sample in `spec-resolve.asm` can be separated at the
last material-composition block:

- `r19.xyz`: emission. Material Part2 byte 48 supplies emission intensity
  (`r23.x`), byte 100 supplies emission multiplier (`r24.xyz`), and material
  texture index `35*material+5` modulates the nonzero result.
- `r15.xyz`: combined reflected environment/specular lighting and accumulated
  local-light specular (`mad r15.xyz, r21.xywx, r29.xyzx, r34.xyzx`).
- `r14.xyw`: diffuse/indirect combination before multiplication by material
  base color. The base-color texture is index `35*material`, multiplied by
  Part2 color multiplier (`r23.yzw`).
- The last `mad` combines material-colored diffuse with specular, then the
  `add r29.xyz, r19.xyzx, r14.xywx` adds emission. The native per-hit cap follows
  this sum, before sample accumulation and division by processed sample count.
- Misses take the environment path and bypass this material-composition block.

These register meanings apply at that final block only; earlier instructions
reuse the same temporaries. This establishes where a contribution-isolation
instrument belongs, but not which contribution causes the captured speckling.

Next implementation decision requires distinguishing emissive hits from
local-light/specular-BRDF contributions, then determining whether importance
sampling can target the dominant bright contributors with correct weights.
The current captures expose accumulated lighting, not per-ray contribution
classes, so they cannot establish which term dominates. No runtime patch is
justified by this audit yet.
