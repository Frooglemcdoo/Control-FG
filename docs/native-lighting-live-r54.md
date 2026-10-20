# R56 direct diffuse/specular diagnostic

R55 successfully isolated the visible speckles to the direct-lighting contribution:
the user's 16:17 session recorded direct_off variant=1 with speckles_gone, then
gi_off variant=1 with speckles_remain. Both contributions had matching bindings.
This narrows the investigation; removing illumination can also hide noise arising
from material response, so it does not prove the lights themselves are defective.

R56 adds DIFFUSE OFF and SPECULAR OFF. They change only the selected direct-light
output multiplier in the exact captured non-RT shader; the other output and baked
GI remain intact. Native, GI Off and full Direct Off remain available.

Use RT/DLSS/RR/FG/glass OFF, the same stationary view, and F10 for the panel.
Compare Native, Diffuse Off and Specular Off. Let each settle and mark Speckles
Remain/Gone. Ctrl+Shift+1..5 selects Native, Diffuse Off, Specular Off, GI Off,
Direct Off respectively. There are no restarts between modes. Save Log shows the
exact filename; the current prefix is Lighting-R56 in the user diagnostics folder.

The captured shader declares UAV0 as g_rwtLightBufferDiffuse and UAV1 as
 g_rwtLightBufferSpecular. At byte 69372, its MAD builds separate x/y multipliers.
Diffuse Off zeros words 6 and 11 (x); Specular Off zeros words 7 and 12 (y).
The original output stores, bindings, sizes and all other instructions are intact.
Checksums and patched hashes are fixed and validated. The mode state reserves
three bits for the five modes and keeps stale-binding rejection. Each alternate
PSO is owned by the native PSO under a separate private-data GUID.

Validation: portable component test verifies the two selected words are the only
payload changes, rejects unsupported identities/instructions, and leaves the
other component's multiplier untouched. Local DXBC checksum/hash checks use the
captured native shader. Windows WARP tests verify selection of distinct native,
full-off, diffuse-off and specular-off PSOs through SetPipelineState and Reset,
with GI unchanged and native restoration. Synthetic WARP outputs validate routing;
the actual in-game visual result remains for this test.

---

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

## R55: cached pipeline coverage

The supplied R54b log recorded 1,824,226 pipeline bindings but zero tagged lighting
bindings. Direct was prepared but never bound; GI was never prepared. It does not
establish that disabling either contribution leaves the image unchanged.

R55 observes LoadGraphicsPipeline and LoadComputePipeline on D3D12 pipeline
libraries, including shared runtime vtables bootstrapped before first use. These
paths reuse the ordinary shader identification and alternate-PSO builders. Shader
hash guards remain exact; no broader shader suppression is enabled. Cache use is
a hypothesis until the new library counters and shader identities confirm it.

The report includes the first binding of up to 512 PSOs with shader hashes/sizes,
or an explicit unobserved-creation marker. This distinguishes unsupported shader
variants from missed creation paths. Pipeline-state-stream creation/LoadPipeline
is not yet covered. Logs now have the Lighting-R55 prefix in the same user folder.
The WARP gate reloads a serialized compute pipeline library, checks the observer
ran and verifies native/off/native GPU outputs for that cached PSO. It uses
synthetic shaders, not Control's actual captured graphics shader.
