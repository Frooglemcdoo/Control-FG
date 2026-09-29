# Known issues — v2.1.2

## RTX 40-series experimental Multi Frame Generation

v2.1.2 includes the NVIDIA 617.14 / DLSS-G 310.9.1 compatibility path for the host-side device-policy check that previously rejected some 3×+ requests with `Found count (2) but expected (1)`.

The compatibility patch is installed only when **Enable RTX 40-series Multi Frame Generation** is enabled before startup. Once installed, it remains active for the lifetime of that Control process, even if FG is temporarily changed to Off or 2×. Disable the experimental option and restart Control to return to the untouched native NVIDIA 2× path. RTX 50-series never uses this patch.

The feature remains experimental. If the exact expected provider contract is not found, Control FG fails closed instead of applying the patch.


## Experimental texture streaming

Options → Experimental includes **Off / 4 ms / 6 ms / 8 ms** texture request budgets. Higher values can improve texture and LOD loading but can also increase per-frame streaming work. **Adjusting this can cause traversal stutter.** Off restores Control's native behavior.

## GOG Galaxy overlay conflict

Disable GOG Galaxy's in-game overlay for Control. It can cause input/focus stalls and make Control FG or other overlays disappear. Close the game, open Control in Galaxy, then **menu beside Play → Manage installation → Configure → Features**. Disable **Use default settings** if shown, then uncheck **Overlay / Access GOG GALAXY features in-game**. Confirm and restart Control. See [INSTALL.md](INSTALL.md#gog-galaxy-disable-the-overlay-for-control).

GOG's validated game binaries are supported; this is a Galaxy overlay compatibility issue.

## RR and indirect diffuse lighting

Blinds can exhibit crawling/noisy streaks when RR and Ray Traced Indirect Diffuse Lighting are both enabled, on RTX 40- and 50-series GPUs. E/F selection does not resolve it. Disable Ray Traced Indirect Diffuse Lighting or RR as a workaround. The reflection clamp slider does not resolve this artifact; it controls specular/firefly clamping. Investigation is ongoing.

## RTX 40-series overlay pacing

Periodic frame-time spikes while the overlay is visible have been reported on an RTX 4070. A confirmed fix is not included. Close the overlay during gameplay if affected.

## Windows security report

A Windows security block was reported on another machine. The exact detection name and affected file have not yet been collected, so the cause and false-positive status are unconfirmed. Download reputation warnings and Defender Antivirus detections are different diagnoses.

For a report, include the detection name, affected filename, file SHA-256, and Windows Security Protection History screenshot. Do not disable antivirus protection. Suspected false positives can be submitted to Microsoft at https://www.microsoft.com/en-us/wdsi/filesubmission.

## HDR transitions

If an HDR change causes ghosting or corruption, restart Control with the desired HDR setting and collect compact logs if it persists. Removing the overlay footer is a UI change, not a new rendering fix.
