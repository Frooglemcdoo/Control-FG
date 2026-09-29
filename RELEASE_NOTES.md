# Control FG v2.1.2 — RR VRAM, Texture Streaming, and RTX 40-Series MFG Fixes

## Highlights

- **Fixed major Ray Reconstruction VRAM retention.** Live RR enable/disable, Model F ↔ E changes, and render-resolution / DLSS-mode changes now retire and release the previous native RR feature epoch instead of leaving old NGX allocations resident.
- Added **Experimental Texture Streaming** controls under **Options → Experimental**: **Off / 4 ms / 6 ms / 8 ms**. Higher request budgets can improve texture and LOD loading, but **can cause traversal stutter**.
- Added a persistent **Show VRAM usage monitor** checkbox in Options.
- Added the **RTX 40-series NVIDIA 617.14 / DLSS-G 310.9.1 compatibility fix** for experimental Multi Frame Generation.
- Added explicit Ray Reconstruction credit to **speedlemur** for the core RR integration approach used by recent Control FG builds.

## RTX 40-series Multi Frame Generation

The experimental RTX 40-series path now handles the newer host-side device-policy validation in DLSS-G 310.9.1 that could reject 3×+ with:

`Multi frame is not supported on this device. Found count (2) but expected (1)`

Control FG validates the exact `EndpointCoreInputs::ComputeAndValidateTimeFactor` contract and changes only the unsupported-device branch so Ada can enter the provider's existing bounded multi-frame path. Existing count, index, resource and time-factor validation remains intact. If the expected contract cannot be verified, the experimental path fails closed rather than applying an unknown patch.

The compatibility patch is loaded **only when Enable RTX 40-series Multi Frame Generation is enabled before startup**. Once loaded, it remains active for that game process even if FG is temporarily changed to Off or 2×. Disable the experimental option and restart Control to return to the untouched native NVIDIA 2× path. **RTX 50-series never uses this patch.**

## Ray Reconstruction VRAM fix

The RR lifecycle now performs explicit feature-epoch retirement across live transitions. This addresses the multi-gigabyte VRAM growth seen when RR was enabled after startup, when switching Model F ↔ E, and when moving between DLSS render resolutions or DLAA.

Starting Control with RR enabled already had a much smaller memory footprint; v2.1.2 brings the live transition path in line with that behavior by releasing stale native RR feature allocations before rebuilding.

## Experimental Texture Streaming

Options → Experimental now includes:

- **Off** — restores Control's native texture streaming behavior.
- **4 ms**
- **6 ms**
- **8 ms**

The higher modes increase the texture-request budget using the validated expanded update-slice path. They can reduce delayed texture/LOD loading and level-streaming stalls, but larger values move more streaming work into a frame and **can cause traversal stutter**. The setting applies live and is saved.

## Installation — Steam, Epic Games Store, and GOG

Download **Control-FG-v2.1.2.zip**. Close Control, extract the ZIP, and copy **dxgi.dll** plus the **complete ControlFGStreamline folder** beside **Control_DX12.exe**. Replace the previous Control FG files together when updating. Launch in **DX12** and press **F10** or your saved shortcut.

- **Steam:** Library → right-click Control → Manage → Browse local files.
- **Epic:** Library → Control's three-dot menu → Manage → folder icon beside Installation.
- **GOG Galaxy:** Control → menu beside Play → Manage installation → Show folder.

The separate **Control-FG-v2.1.2-Source.zip** requires compilation.

## Compatibility and known issues

Validated targets remain **Steam 21225456**, **Epic 0.0.518.2177**, and **GOG EXE SHA-256 beginning 57A8912F**. DX11 and unvalidated game updates are not supported.

GOG users should disable Galaxy's in-game overlay for Control. RR with **Ray Traced Indirect Diffuse Lighting** can still show crawling/noisy streaks; disable that game setting or RR as a workaround.

## Credits

A massive thank you to **HotKnives** for lending hardware and helping with QA around recent releases and bug fixes.

A big thank you to **speedlemur** for the core Ray Reconstruction integration approach used by recent Control FG builds. That work is credited in the project README, Nexus copy, and notices.
