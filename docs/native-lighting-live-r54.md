# R54 live lighting diagnostic

Supersedes R53's startup markers and restart-based selector. Install with the
game closed, launch once, then change modes live. Defaults to native each launch.
Old R51/R52/R53 marker files are ignored.

## Controls and one-file report

Run Control in Borderless mode. R54a fixes the original panel's mouse ownership:
the panel starts hidden; press F10 to open it and give its buttons keyboard/mouse
focus. Press F10 again to hide it and return focus to Control. Tab/Space also
operate the buttons. The panel releases cursor confinement while it owns focus
and balances its cursor-visibility changes when focus leaves. It does not steal
focus back from another app. Alt-Tab hides it; F10 reopens it from the game.

The shared UI theme/fonts and three mode buttons (Native, GI Off, Direct Off)
remain. Ctrl+Shift+1/2/3 also changes modes with the game or panel foreground.
There is no global input hook, game-window subclass or permanent topmost window.
Exclusive fullscreen is unsupported. No pause command is sent; if Control stops
rendering while its window lacks focus, close the panel with F10 to let the new
mode render, then reopen it to record the result. No game restart is needed.

Use the same stationary stair-wall view with RT/DLSS/RR/FG/glass OFF. Wait for
bindings recorded and allow exposure/history to settle, then use Speckles
Remain or Speckles Gone to record your visual observation. Repeat for each mode
and press Save Log. R54b confirms the exact saved path in a dialog, or shows the
Windows error if writing fails. Reports go to `%LOCALAPPDATA%\Control-FG\Diagnostics`,
falling back to `%TEMP%\Control-FG\Diagnostics` if needed. Each launch creates one
unique `Lighting-R54b-<date>-<time>-<pid>-<serial>.log`; previous sessions are kept.
The ordinary mod log receives the same diagnostic events.

Rejected mode clicks now explain that the matching shader is not ready. A selected
mode shows NOT APPLIED until its tagged pipeline has been recorded. This is not
GPU-completion proof or proof that all lighting in the scene uses this pipeline.
Save Log includes graphics/compute PSO and binding counts, same-size shader hash
mismatches, variant creation results and hook status to diagnose missing coverage.
The unchanged-image report from R54a does not establish that direct lighting is
unrelated to the speckles: successful application has not yet been verified.

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

R54b adds Windows file tests for an unwritable primary destination, fallback,
readable flushed contents, append preservation, unique sessions and total failure.
