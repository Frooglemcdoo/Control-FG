# R485 REBLUR integration checkpoint

Not a game-ready build. The full-scene reflection adapter is not installed.

## Implemented

- Two-choice menu: REBLUR (RR off), REBLUR before RR. Existing persisted RR boolean selects the mode. RR still processes all characters in the second mode.
- REBLUR diffuse session and separate specular session/resource bindings; conversion uses YCoCg and normalized hit distance. Specular normalization uses material roughness, diffuse uses 1.
- Mode changes allocate a new diffuse history epoch; outstanding recordings retain their existing epoch until retirement.
- History windows: 250 ms accumulated, 50 ms fast, 100 ms stabilized, with bounded frame counts. Default stabilization failed the removed-light regression.
- Build option consistently defines REBLUR for C++ and conversion shaders.
- Disabled scene diagnostics activity and capture UI in this build; additionally disabled startup/resume texture sampling and swapchain/HUD capture paths. Full build flag audit remains required.

## Verified

Windows run 44624559242 at commit 0e4f8d54d840e7f29c5c14c8d9e0dae36362151f passed all five tests: contract, diffuse raw backend, diffuse conversion pipeline, specular raw backend, specular conversion pipeline. D3D12 WARP debug layer was clean. These are synthetic GPU integration checks, not NVIDIA GPU performance or in-game quality tests.

After light removal, expected 0.01; diffuse pipeline measured 0.01054408 and specular pipeline 0.01055795. Initial defaults measured approximately 0.038 and failed the unchanged 0.025 limit. Motion/disocclusion and native binding restoration checks also passed. CPU contracts pass with both REBLUR and legacy RELAX selected. Menu/schema checks pass.

## Remaining before packaging

1. Connect the independent specular pipeline at the native reflection temporal boundary, in both RR modes, preserving native BRDF composition. Do not reuse diffuse receiver throughput as a reflection guide.
2. Authenticate current-frame reflection hit distance, roughness/world normals, depth and motion at that boundary. Native temporal specular bindings include depth, motion, source/history/target, but do not themselves supply a ray hit-distance texture. Existing pre-RR distance code runs later and cannot simply be called at the earlier boundary.
3. Retire specular resources through the native queue/fence lifecycle, reset on mode/resolution/view changes, and bypass only the native filtering successfully replaced.
4. Wire full game build to the new CMake/shader options and audit capture-free flags; compile the proxy and run full regression checks.
5. Package only after full integration. Do not distribute validation executables as the mod or label diffuse-only integration as whole-scene REBLUR.

Native shadow denoising remains separate: REBLUR handles diffuse/specular radiance, not shadow masks. No claim is made that Jesse's red speckles are fixed.
