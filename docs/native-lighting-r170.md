# R170 — parallel SPEC 16 shading test

This experiment retains all 16 reflection ray samples and changes how one captured native compute shader shades those hits. It does not change ray generation, material/lighting instructions, the per-sample clamp, sample accumulation order, or the accepted R168 spotlight shaders. RR remains governed by the existing settings.

## Test

1. Close Control and install the ZIP beside Control_DX12.exe, preserving the NVIDIA runtimes and ControlFGStreamline directory.
2. Start a fresh session. Use NATIVE BLEND, RR on, FG off, and SPEC: 16.
3. Compare PARALLEL: ON and OFF in the same view. Check paintings, glossy materials, edges and movement. OFF uses the original serial shader with the same 16 samples.
4. Run PERF CAPTURE once in each state, using the same view or movement route. Send both ZIPs. No restart is needed between comparisons.

SPEC defaults to Native on restart. PARALLEL starts ON but is used only when SPEC 16 is requested and the exact captured native shader has a compatible replacement. Other sample counts use native scheduling. Missing or unsupported shaders retain the native path.

## Implementation

The native 8 x 16 group originally shades 128 pixels serially over their sample lists. The replacement shades eight pixels, with 16 sample lanes per pixel. Sixteen Z groups cover the same original 16 pixel rows. Each group uses 2 KiB of shared memory for hit colors and validity flags. Every thread reaches a common shared-memory barrier; one lane then sums valid samples in the native order and runs the original normalization and final output instructions.

The source SHA256 is pinned to 4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9. The transform preserves the native hit shading and clamp instruction bytes. Every lane observes the first 0xfffe material sentinel, even if later texture layers contain valid data. Lanes beyond the requested count skip sample-array access. Out-of-bounds pixel lanes still reach the barrier.

The native PSO owns the alternate PSO. Command-list binding metadata follows SetPipelineState and Reset. Only the exact bound target can use the alternate, and the original PSO is restored immediately after dispatch. There is no additional full-screen intermediate texture, render-thread file I/O, CPU/GPU wait or periodic capture work.

## Validation and limits

The build gates on hash rejection, unchanged lighting-body tokens, uniform barrier placement, Windows reflection/root-signature/PSO validation, and WARP pixel-by-pixel comparison of native and parallel scheduling. Differential fixtures replace only hit shading with positive sample/pixel-dependent colors and retain the real clamp, sentinel handling, accumulation and final output. Cases cover 0/1/2/4/8/12/16 samples, enabled/disabled clamp, partial edge groups, and sentinels followed by valid entries. These fixtures validate scheduling; they are not an in-game visual validation or a GPU performance result.

No performance gain is claimed before measurement. More parallel work may increase scheduling overhead or reduce cache locality. This experiment does not reduce the separate reflection ray-generation cost.

Capture manifests include requested parallel state, PSO readiness, cumulative recorded dispatches and the dispatch count during measurement. A requested ON state alone is not proof the shader ran. Sparse timing CSV dimensions describe incoming native dispatch arguments; this shader's parallel dispatch expands Z from 1 to 16. Counts record CPU command recording, not proof of GPU execution by themselves.
