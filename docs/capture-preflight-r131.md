# R131 capture driver preflight

Keep the working R129 mod installed. This ZIP is a standalone test, not a game
update. Extract it into its own folder outside Control, close the game, and run
`Run-Capture-Preflight.cmd`. No admin rights, Python, PIX, or .NET setup is needed.
Send the single `Capture-R131-*.log` whose location is printed at the end. Logs
are saved automatically under `%LOCALAPPDATA%\Control-FG\Diagnostics` with a
temporary-folder fallback when LOCALAPPDATA is unavailable.

The test uses the preferred hardware adapter and logs its identity and driver
version. Each of five stages runs in a separate child process:

1. Create the reference R97 RT pipeline.
2. Create the shared capture hit pipeline, retaining the native alpha handler.
3. Create the repaired R131 ray-input pipeline.
4. Create translation pipelines with the live native root, then run bounded GPU
   table translation/readback against independently computed expected bytes.
5. Allocate capture resources and exercise the real descriptor hooks outside
   capture scope, as occurs during loading.

The global root is byte-identical to the R129 log (SHA-256
451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3).
The local root reproduces the recorded three-parameter layout. The reference
shader is the authenticated R97 library, containing the original hit handlers.
Pipeline factories and translation tests are shared with the capture source.
The preflight build flag is confined to this executable; gameplay remains gated.

D3D12 validation is enabled before creating a device when Windows Graphics Tools
is installed. Its absence is logged and does not prevent the hardware check.
Each stage flushes its operation marker before entering pipeline creation.
The parent retains the log if a child crashes and stops after a 45-second stage
timeout. A process boundary protects the game; it cannot guarantee recovery
from a driver-wide GPU failure.

This tests startup and bounded compute, not game-scene ray traversal. It does
not attach to Control, load its DLL, trace rays, or modify any game files. A
successful result would narrow the remaining investigation to game/interposer
integration; a failing stage gives us a concrete driver-side operation to fix.

CI compiles the Windows executable and runs the parent/child workflow on WARP.
WARP DXR skips are explicitly marked, never counted as hardware validation.
The next user test is needed because CI has no RTX 5090.

## R131 repair candidate

The user's R130 test passed reference and hit-pipeline creation on RTX 5090
(driver 32.0.16.1742), then crashed with 0xC0000005 inside CreateStateObject for
the input pipeline. No ray dispatch or descriptor-hook test had run.

R131 removes two unused ShadowHitData stack allocations, their address
calculation and payload initialization store from the input-only export.
R128 had moved those allocations into a conditional block when it wrapped the
native prefix with bounds checks. The four instructions no longer had any
consumer after TraceRay was removed. This is a concrete cleanup and compiler
compatibility repair candidate, not proof of the driver's internal fault.

The extracted shader is now passed through DXC's single-export library linker.
The shipped library contains only captureNativeInputs, with no payload storage,
hit/miss exports, TLAS binding, native output UAVs or local-root material data.
Tests verify the extraction changes only those four payload instructions;
origin, direction, selected light, endpoint, pixel mapping and record ABI remain.
The compiler-linked library is validated separately and its hash is pinned.

The in-game capture stays disabled until this corrected pipeline passes on the
5090. The old crashing input binary is not executed as an A/B test.
