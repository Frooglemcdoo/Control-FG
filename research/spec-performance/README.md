# R169 SPEC 16 compute audit

## Verified evidence

The second SAVE LOG export (Lighting-R169-20261026-153329-66068-0.log) contains 346 complete shaders, zero dropped. The compute shader in this directory is 42,408 bytes with SHA256 4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9. Windows D3DDisassemble succeeded in Actions run 40397536284.

The preceding Diagnostics3 capture measured this source shader at 5.69003 ms with SPEC 16 and 0.42869 ms with Native sampling. These are sparse diagnostic GPU timings, not uninstrumented benchmarks. The reported native/source PSO identity must not be confused with proof of every possible runtime replacement. The exported shader establishes the native workload.

## What the shader does

The shader uses 8 x 16 threads per group, 43 temporary vector registers, and approximately 941 static instruction slots. Dispatch dimensions were 320 x 90 x 1. These are compute thread groups, not ray counts.

Each in-bounds pixel executes an outer sample loop. It checks the material sentinel 0xfffe and CB6[0].z sample limit; loads per-sample position and normal/UV textures; evaluates material textures, baked illumination volumes, environment lighting, local point/spot lights and shadow maps; applies the per-sample luminance clamp; and adds to the pixel accumulator. The loop advances the texture-array slice and material ID. After the loop, the shader divides by the processed sample count, applies the native final multiplier, and writes one output pixel.

Many invariant inputs already sit outside the sample loop, including ambient SH loads. Distinct hits can have different position, material, normal, UV and lighting. No redundant whole lighting evaluation has been established. The centered spotlight identical-ray optimization cannot simply be reused here.

## Historical R170 experiment (rejected)

Evaluate parallel processing of the existing hit samples rather than lowering their count or reusing temporal results. Preserve the native sentinel termination rule, material and lighting evaluation, per-sample clamp, final multiplier, resource bounds and sample ordering for accumulation. Keep all 16 ray-generation samples and existing hit-array allocation coherent.

A parallel implementation requires an audited translation or transformation of this shader, intermediate per-sample results, and ordered reduction. It is not a thread-group-size-only patch. Additional memory traffic, register occupancy and wave divergence may erase the gain. Validate output against the captured native shader on synthetic material/sentinel/clamp cases and on-device scenes before claiming equivalence or performance. Keep R168 shadow behavior and RR policy unchanged.

No rendering change or optimized game build is delivered by this audit. No FPS gain is claimed.

## Current follow-up

R170 parallel scheduling regressed on the user's GPU and R171 removes its runtime path. R172 instead shortens two material-load live ranges while preserving serial scheduling. See `../../docs/native-lighting-r172.md` for the exact instruction movement, validation limits and required same-settings R171/R172 comparison. No measured gain is established.
