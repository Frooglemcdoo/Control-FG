# Control FG roadmap

## v1.0.0 — Frame Generation + Ray Reconstruction

The first public release now includes the stable Control-specific DLSS Frame Generation path and working DLSS Ray Reconstruction.

Completed for v1.0.0:

- fixed 2x–6x Frame Generation modes on supported hardware;
- native Dynamic Multi Frame Generation;
- live HDR/SDR transitions with hard DLSS-G resource reset/rearm;
- persistent F10 overlay and runtime status;
- Control-native HUD-less scene capture and UI recomposition for generated frames;
- DLSS Ray Reconstruction integrated at Control's native RT denoising boundary;
- RR runtime 310.9.1 with Preset F default and live E/F switching;
- compatibility testing across DLSS mode changes, ray tracing, HDR and Frame Generation.

## Future: RTX 20/30-series Frame Generation and Multi Frame Generation

Post-v1.0.0 research will investigate extending NVIDIA Frame Generation / Multi Frame Generation support to **RTX 20-series (Turing)** and **RTX 30-series (Ampere)** hardware.

Recent community work has demonstrated that this is technically possible by combining architecture-gate handling with GPU-specific DLSS-G kernel/backend work. Control FG will treat this as a separate compatibility project so the stable v1.0.0 FG/RR path is not destabilized.

Research areas include:

- early Streamline/NVAPI architecture gating so the DLSS-G plugin is not rejected before initialization;
- Turing/SM75 and Ampere/SM86 execution paths;
- exposing higher MFG multipliers where the runtime and hardware path prove stable;
- preserving Control FG's existing HUD-less scene capture, HDR lifecycle handling, Reflex pacing and dynamic multiplier logic;
- real-hardware validation before any public support claim.

Reference project for this research:
- `sdli1995/dlssg_for_sm86`

This work is **not included in v1.0.0**.

## Next: DLSS Super Resolution module

A separate DLSS Super Resolution module is also planned. The goal is to expose selectable NVIDIA DLSS SR runtime/model families rather than tying Frame Generation or Ray Reconstruction development to one upscaler path.

Planned research includes:

- selectable DLSS Super Resolution runtime/model families;
- newer transformer/model variants where the game inputs and NVIDIA runtime support permit them;
- a clean Control-styled UI integrated alongside FG and RR controls;
- compatibility checks and safe fallback behavior.

## Longer-term direction

The project is being developed as a set of game-aware rendering modules rather than a single isolated Frame Generation patch. The longer-term goal is to preserve Control-specific renderer knowledge — including HUD-less scene capture, motion vectors, depth, camera state, HDR handling and presentation timing — while adding additional DLSS/FG capabilities where they can be integrated cleanly and validated in runtime testing.
