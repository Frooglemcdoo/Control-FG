# R171 — restore serial specular shading

R170 parallel shading regressed performance in the RTX 5090 test. R171 removes the experiment from the production runtime: no alternate specular PSO creation, no parallel metadata reads/writes on pipeline bind/reset, and no parallel dispatch selection. Compute dispatch forwards the original X/Y/Z once. The overlay reports SPEC SHADING: SERIAL; that control cannot enable parallel shading.

All 16 reflection samples, the native hit shading/clamp/accumulation, the accepted R168 spotlight shaders, the existing GI pass control (use 6), and RR policy are preserved. This removes the R170 experiment and its CPU bookkeeping even compared with R170 Parallel OFF. It does not optimize the original ray tracing or claim a measured FPS gain.

## Test

Close Control and replace package files beside Control_DX12.exe, preserving NVIDIA runtimes and ControlFGStreamline. Restart, select NATIVE BLEND, GI 6, RR on, FG off and SPEC 16. Check the same view and motion route used for R170. Run PERF CAPTURE at SPEC 16 and SPEC Native without changing other settings. SPEC resets to Native at restart.

Capture records identify R171 and retain parallel fields for comparison; enabled, ready and dispatch counts must remain zero. The original shader is used, so no shader-quality approximation is introduced. The historical parallel transform and GPU fixtures remain test-only and are not included by the production runtime. Windows CI builds and validates the package. In-game performance and visual confirmation still require testing.
