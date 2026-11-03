# Full GI replacement — editable evaluator boundary

## Accepted direction, 2026-11-03

The user explicitly selected full GI replacement, not replacing direct point/spot
lighting or shadows. Green speckles in the latest room are deferred. Preserve the
R308 sampler/native toggle, authored lights, materials, DirectStorage and RR.

R308 PID 63964: capture part SHA256 and all three indexed files validate. The log
records GI_TOGGLE_R308 ready=1 variant=0, and actual recorded new/native/new/native/
new dispatch choices for revisions 0 through 4. RAW BYPASS was requested. The user
reports the inspected areas clean except green speckles in one space. These logs
confirm route selection, not GPU completion or measured image equivalence. No
additional capture is needed to repeat that route validation.

## Implemented source boundary

`generate-gi-evaluator.py` recovers the entire captured GI6 evaluator as HLSL from
SHA256 1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de.
All 674 executable instructions are represented once, in order, with source
addresses. The source keeps the 8x16 thread group, descriptor registers/spaces,
material arrays, point/spot evaluation, shadow atlases and cookies, volume
fallback, ray denominator, depth fade, output alpha and hit-info output.

The earlier PT translator is reused as a starting point. Its PT-specific range
selection, direction overrides, material mip override, emission extraction and
finite/nonnegative return clamp are NOT included. GI loads use the DXBC W mip
component for 2D resources; array loads retain Z as slice and W as mip. Integer
signed comparisons, bit reinterpretation, source modifiers, masked writes and
resource swizzles remain explicit. Nonzero sample-offset extensions are rejected
rather than silently dropped. The checked-in HLSL is reproducible from captured
bytes; unknown hashes fail before output creation.

This reference port is NOT the replacement estimator, is not bound into the game,
and is not claimed bit-identical in GPU execution. DXC recompilation can alter
floating-point optimization. Register initialization is explicit in the port;
possible native undefined inputs and out-of-bounds behavior need GPU comparison.
No new game ZIP or user capture is justified just by successful compilation.

## Next integration work

1. Validate resource-bound reference execution against the captured native
   evaluator, including hit/miss, local lights and volume fallback. Prefer
   existing captured data and automated WARP fixtures. Compile/structural tests
   do not replace image/output comparison.
2. Separate secondary illumination from source-dependent BRDF/throughput in the
   owned evaluator. Supply actual proposal density from the R307 producer;
   retain all-ray normalization and separate miss fallback.
3. Integrate spatial reuse with true accepted geometry, current native alpha-aware
   connection visibility, destination shading and correct normalization. The
   existing geometry/owner/RTXDI adapters remain available. Do not interpret
   smoothed vertex normals as geometric normals or use source-weighted GI RGB
   as reusable secondary radiance.
4. Add temporal reuse after spatial reuse is valid, with disocclusion and RR
   compatibility checks. Keep the native comparison available throughout.

NVIDIA describes ReSTIR GI secondary radiance as non-directional, unlike terms
in this native evaluator. That approximation must be explicit in the replacement,
not obtained by quietly relabeling the current output:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/ShaderAPI-RestirGI.md
DXBC LD mip/slice semantics:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/ld--sm4---asm-

## Validation

Six local tests pass: reproducibility, all-instruction coverage, mip/slice
handling, outputs/normalization, unmodified material sampling and wrong-hash
rejection. DXC compiles the complete shader as cs_6_0, HLSL 2018, with warnings
as errors. Windows foundation CI now includes these checks. This is compiler and
source-contract validation, not a quality or speed result.

Windows foundation run 41693404033 / job 126117527584 passed at remote commit
6ed98f204a2f6cb43e133e1a6475786784a1ddc7. This includes the complete GI evaluator
compile and translation tests plus the existing CPU/WARP foundation suite. The
GI evaluator itself is not executed by those existing WARP adapter tests.
