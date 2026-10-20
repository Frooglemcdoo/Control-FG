# R54 live lighting diagnostic

Supersedes R53's startup markers and restart-based selector. Install with the
game closed, launch once, then change modes live. Defaults to native each launch.
Old R51/R52/R53 marker files are ignored.

## Controls and one-file report

Run Control in Borderless mode. The owned, nonactivating diagnostic panel uses
the shared UI theme/fonts and shows three mode buttons: Native, GI Off and
Direct Off. F10 shows/hides this diagnostic panel. Ctrl+Shift+1/2/3 offers the
same live selection when the game owns the mouse. The panel has no global
input hook, game-window subclass, forced foreground activation or permanent
topmost status. It hides when Control loses foreground. Exclusive-fullscreen
visibility is not supported by this Win32 panel.

Use the same stationary stair-wall view with RT/DLSS/RR/FG/glass OFF. Wait for
bindings confirmed and allow exposure/history to settle, then use Speckles
Remain or Speckles Gone to record your visual observation. Repeat for each mode
and press Save Log. Send only `ControlFG-Lighting-R54.log` beside Control_DX12.exe.
It is flushed as events occur and recreated on each launch, so send/copy it
before launching again. The ordinary mod log receives the same diagnostic events.

The dedicated log contains shader-variant readiness/failures, mode sequence,
recorded native/alternate bindings, present counter/time, user observations and
save markers. It does not infer visual quality automatically. A binding log
confirms command recording, not completed GPU execution. A selection lacking
matching bindings must not be treated as a successful isolation test.

## Live implementation

Both exact R53 bytecode transformations are retained, guarded by complete native
and patched hashes and instruction checks. Native pipeline creation always
returns the original pipeline to the game. A second pipeline is prepared once
for each matching PSO. It is attached to the native PSO through D3D12 private
interface data, which owns its reference. There is no accumulating global PSO
cache or device pinning. Query references are released after recording.

Device CreateCommandList observation installs narrow SetPipelineState and Reset
hooks on encountered command-list vtables, retaining their original functions.
Initial pipeline arguments to CreateCommandList are handled as well. At command
recording, only a tagged native PSO for the selected contribution is substituted.
Native mode passes the original PSO. Other pipelines and lighting passes remain.
Changing modes requires no shader compilation, resource reallocation or restart.
Previously recorded/in-flight work can finish with its old selection; wait for
the new bindings and settled image before evaluating it. The mode and sequence
share one atomic state so an old worker cannot confirm a newer selection.

GI Off removes only the captured illumination-volume contribution. Direct Off
removes only the captured deferred-light diffuse/specular contribution. Other
passes remain; these are not exhaustive pure lighting views. Exposure and
history may change as the image becomes darker. Unsupported shader variants
are unchanged, so test the same non-RT configuration as the original capture.

## Validation

The portable policy test checks mode routing, native restoration and stale
worker rejection. Windows WARP testing renders known values through actual
hooked command lists, toggling all three modes and returning to native through
both SetPipelineState and Reset. This validates switching/ownership machinery
using synthetic shaders, not the visual effect of the game shaders on NVIDIA.
The exact game shader patches were separately checked byte-for-byte in R53.
The 132-file clean-baseline audit permits only the documented diagnostic hook
integration. Existing glass, menu and provider checks remain enabled.
