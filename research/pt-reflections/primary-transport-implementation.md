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
