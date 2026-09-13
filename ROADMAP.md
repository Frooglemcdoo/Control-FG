# Control FG roadmap

## v1.0.0 — Frame Generation

The first public release is focused on a stable DLSS Frame Generation experience for Control DX12:

- fixed 2x–6x modes on supported hardware;
- native Dynamic Multi Frame Generation;
- live HDR transitions;
- persistent F10 overlay and runtime status;
- capability-aware mode restrictions.

## Current development: DLSS Ray Reconstruction

Ray Reconstruction is now an active development target and is making good progress.

The goal is to integrate NVIDIA DLSS Ray Reconstruction directly into Control's existing DX12 ray-tracing path rather than treating it as a generic post-process replacement. Current work is focused on identifying and validating the native game/NGX integration points, proving the resources and state Ray Reconstruction needs, and building the runtime path carefully without destabilizing the production Frame Generation core.

Current areas of work include:

- validating Control's native NGX and ray-tracing integration points;
- identifying the denoiser replacement boundary used by the game's existing RT pipeline;
- proving the depth, motion-vector, camera and ray-tracing resources required by Ray Reconstruction;
- integrating the required Streamline / NGX feature path while keeping the existing Frame Generation implementation stable;
- testing compatibility with live DLSS mode changes, ray tracing, HDR and Frame Generation;
- building runtime validation and fallback behavior before exposing Ray Reconstruction as a public feature.

Ray Reconstruction will remain marked as **in development** until image quality, stability, resource lifetime, synchronization and compatibility are proven in real gameplay. The project will continue to distinguish confirmed runtime behavior from experimental work.

## Next: DLSS Super Resolution module

A separate DLSS Super Resolution module is also planned. The goal is to expose selectable NVIDIA DLSS SR runtime/model families rather than tying Frame Generation or Ray Reconstruction development to one upscaler path.

Planned research includes:

- selectable DLSS Super Resolution runtime/model families;
- newer transformer/model variants where the game inputs and NVIDIA runtime support permit them;
- a clean Control-styled UI integrated alongside FG and future RR controls;
- compatibility checks and safe fallback behavior.

## Longer-term direction

The project is being developed as a set of game-aware rendering modules rather than a single isolated Frame Generation patch. The longer-term goal is to preserve Control-specific renderer knowledge — including HUD-less scene capture, motion vectors, depth, camera state, HDR handling and presentation timing — while adding additional DLSS/FG capabilities where they can be integrated cleanly and validated in runtime testing.
