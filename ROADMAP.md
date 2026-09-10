# Control FG roadmap

## v1.0.0 — Frame Generation

The first public release is focused on a stable DLSS Frame Generation experience for Control DX12:

- fixed 2x–6x modes on supported hardware;
- native Dynamic Multi Frame Generation;
- live HDR transitions;
- persistent F10 overlay and runtime status;
- capability-aware mode restrictions.

## Next: DLSS Super Resolution module

After the first Frame Generation release is stable, development moves to a separate DLSS Super Resolution module. The goal is to expose selectable NVIDIA DLSS SR runtime/model families rather than tying FG development to one upscaler path.

Planned research includes:

- selectable DLSS Super Resolution runtime/model families;
- newer transformer/model variants where the game inputs and NVIDIA runtime support permit them;
- a clean Control-styled UI integrated alongside FG controls;
- compatibility checks and safe fallback behavior.

## Later: Ray Reconstruction

Ray Reconstruction research follows the SR module. It will be treated as a separate rendering integration project and will not be advertised as supported until Control's ray-tracing inputs, denoiser replacement points and required Streamline resources are proven in runtime testing.

The project will continue to keep measured runtime results separate from experimental/planned features.
