# Primary surface transport implementation, first module

Implemented in `src/shaders/pt_surface_transport.hlsli`. This is shared HLSL/C++
code; the C++ tests execute the actual transport formulas, not a translated test
model. It is not included in any live Control shader or runtime hook yet.

## Implemented

- Lambertian diffuse and isotropic GGX/Schlick specular evaluation.
- Cosine diffuse sampling and visible-normal GGX sampling, using a mixture PDF
  for the combined BSDF. Below-surface GGX samples terminate without resampling.
- Material-weighted throughput `f * cosine / pdf` and radiance accumulation.
- Russian roulette with survival compensation; normal termination is distinct
  from invalid data, so a future compositor can reject a broken path result.
- Explicit path depth, live state and validity. A depth limit is caller-owned.

The GGX visible-normal sampling reference is Eric Heitz, JCGT 2018:
https://jcgt.org/published/0007/04/01/
The sampler follows the same visible-normal approach already used in the
existing reflection shader. The new evaluator uses the full mixture probability,
so diffuse continuation is not simply added onto a specular estimator.

## Material contract and limits

Directions use an orthonormal local frame, +Z along the shading normal. The
caller supplies normalized directions and the geometric-normal admissibility
test. No shading-normal correction, transmission, exact delta mirror, anisotropy,
cloth, hair or multiple-scattering compensation is implemented here.

`kd` is an admitted diffuse coefficient, with the material's energy partition
already applied; it is not an arbitrary G-buffer channel. `f0` is reflectance.
Both must be finite in [0,1]. The caller must ensure their combination is a
physically appropriate material. Independent maxima do not enforce total energy
conservation. Roughness is clamped to [0,1], and squared alpha has a 0.001 floor.
This finite-width limit is intentional; exact mirrors need another sampling path.

For the reviewed primary composition contract, accumulate reflected primary
radiance, excluding native primary emission and fog. At later hits include
transported emission as appropriate. Direct-light samples and emissive/miss
contributions require caller-provided visibility, PDF and MIS accounting.
`PTAccumulate` does not invent that information or substitute native cached GI.

## Validation performed

Compiled the HLSL fixture to cs_6_0 with DXC. Compiled and ran the C++ fixture.
Twelve tests compare sampled estimates to numerical hemisphere integrals at
three roughnesses, normal/grazing views, and specular-only/mixed lobes. Each uses
350,000 samples; below-surface samples count as zero rather than being discarded
from the denominator. All passed. The largest observed absolute estimate versus
quadrature difference was below 0.00034 for these cases.

Additional checks passed for analytic Lambertian weights, throughput applied
once during accumulation, roulette expectation, back-facing termination and
invalid-data rejection. These establish internal sampling/evaluation consistency,
not native material parity or GPU execution parity. The reference quadrature
uses the same BSDF evaluator but an independent integration measure, so it cannot
prove the evaluator matches Control's material model.

```text
g++ -std=c++17 -O2 tools/test-pt-surface-transport.cpp -o test-pt-surface-transport
./test-pt-surface-transport
dxc -T cs_6_0 -E main tools/test-pt-surface-transport.hlsl -Fo transport-test.dxil
```

The HLSL fixture's t0/u0 bindings are test inputs/outputs only and are not a
proposed native binding layout.

## Integration remaining

Geometry intersection/hit decoding, admitted native material evaluation, light
selection and traced visibility, emitter/environment MIS, world/local transforms,
ray-origin offsets, random dimensions per frame/bounce, resource allocation and
barriers, history/denoiser guides, and the reviewed composition adapter are still
required. The existing native four-byte payload and hit-table side effects must
not be silently reinterpreted as this path state. This module does not change the
installed mod or provide a playable full-PT build.

## M12R2 follow-up to the failed M12R1 game run

R1's user-reported ~20 FPS is an improvement over 3–5 FPS, but its captured log
contains no PT_PRIMARY_RECORDED/SUBMITTED/GPU evidence. The final CPU totals were
11,570 build callbacks / 45,854 ms aggregate / 188 ms maximum, with nonblocking
checkpoint callbacks. This is cumulative CPU time, not GPU time or per-frame cost.

R2 adds a bounded 64 MiB idle geometry-buffer pool in addition to the existing
512 MiB active snapshot budget. Storage returns resources only after all recording,
scene and submitted-flight owners release it. Each checkout records a fresh copy,
with the same source-state checks and restoration. Retirement now shares one fence
across a native submission batch. Token lookups use a maintained reverse owner map.
An invalid catalog skips new expensive build capture while native calls continue.
R2 logs catalog health, skipped work, copy failures and pool allocation/reuse counts.

The captured native primary DXBC RDEF marks inverse-screen (+8), inverse-output
(+24), ClipToView (+224) and current frame (+1112) as used. Screen XY (+0) is
unused. R2 derives integral render extents from inverse-screen, ignores unused
screen/output fields for view equality and uses current frame for its private RNG.
RDEF unused does **not** prove bytes are absent: captured shader metadata alone
cannot establish the runtime contents of these upload slices. Therefore consumed world-transform components still must be finite and form an affine
inverse pair; ClipToView must be finite and nonsingular. These consumed fields
must match across passes (unused world .w lanes and ViewToClip are ignored); missing transforms cannot silently admit an incorrect camera. Exact field
failure reasons are now logged. If those guards reject, additional provider/camera
integration remains necessary; R2 must not be described as confirmed primary PT.

Validation includes the inverse-extent/matrix fixture and Windows WARP GPU-copy
retirement checks extended to cover pooled reuse only after completion, plus the
existing protected P2/G3 encoding checks. In-game FPS/coverage remain unverified
until this package is tested.
