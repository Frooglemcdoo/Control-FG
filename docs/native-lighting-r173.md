# R173 — apply serial SPEC optimization to pipeline-cache loads

R172 remains the user-reported performance baseline: 5–7 FPS recovered. Its submitted capture measured 42.2 presents/s at effective SPEC 16, GI 6, RR selected and FG off, with zero timing faults/errors. Seven samples of the source-identified reflection resolve averaged 6.125536 ms. The performance archive's SHA256 matches its submitted sidecar. No matched before/after capture establishes which R172 change caused the gain.

## Finding and change

R172 substitutes its hash-locked serial shader only in CreateComputePipelineState. The existing ID3D12PipelineLibrary::LoadComputePipeline hook explicitly observes native cached PSOs that bypass device creation. That path did not apply the R172 load-lifetime optimization. Missing startup log lines alone do not prove that the submitted run used this path, because performance capture copies log tails.

R173 uses a shared exact-hash creation helper for both routes. A successful native library load remains the fallback. Only if replacement creation succeeds is its returned COM reference substituted and the native returned reference released. Failed loads, unsupported shaders, and failed replacement creation preserve native behavior. The library's stored entry is not modified. The original device creation function is called directly, avoiding hook recursion.

The shader bytes match R172 exactly (SHA256 c4bebcb782c972985e77ea22c04dd8301832b36f863379f3f041a1801ed02889). All 16 samples, serial dispatch, accumulation, clamp, GI, accepted spotlight shaders and skin/hair guides are unchanged. No parallel dispatch or per-bind metadata is added. This enables the previous candidate on an uncovered path; it is not a new shading approximation or a guaranteed gain.

## Evidence and validation

Every PERF CAPTURE settings snapshot now includes direct/cache target-match and successful-creation counts plus replacement failures. These counters survive startup-log truncation. They establish creation, not proof of GPU execution. Target/source shader timings keep the original source identity, as before.

Local tests cover direct/cache helper routing, exact-target rejection, descriptor preservation, failed/null-result fallback, unexpected failed-object cleanup and counters. The existing two-load movement audit still passes. Windows CI runs the helper test, shader reflection/root/PSO validation and project checks. Actual performance and visual equality require in-game comparison.

## Test

Close Control, install R173, restart, select NATIVE BLEND, SPEC 16, GI 6, RR on and FG off. Repeat the same view/route and PERF CAPTURE used for R172. Keep shader caches intact: the cached path is the purpose of this change. Compare reflection detail, lighting, paintings, glass and hair during movement.

Read spec_serial in manifest.txt: library_created greater than zero confirms a successful cached-route replacement; direct_created greater than zero confirms direct-route creation. Both zero means this optimization has not been confirmed applied by either route. No FPS improvement is claimed until measured. Keep R172 as the comparison build.
