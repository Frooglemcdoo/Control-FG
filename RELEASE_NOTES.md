# Control FG v2.1.1 — Steam, Epic Games Store, and GOG

## New in v2.1.1

- **Epic Games Store and GOG are now supported alongside Steam in one deployment package.**
- Retains complete matching EXE/D3D/renderer hash checks for each validated storefront; unknown or mismatched builds are rejected.
- Adds installation and update instructions for all three stores.
- Documents the GOG Galaxy overlay conflict and how to disable the overlay for Control.
- Updates the overlay version, runtime logs, source metadata, and package identity to **2.1.1**.

Based on the unified R3 build tested on all three stores. Existing FG, experimental RTX 40-series MFG, RR Model F/E, HDR recovery, and settings behavior are retained. This release does not include a new RR noise fix.

## Installation — all three stores

Download **Control-FG-v2.1.1.zip**. Close Control, extract the ZIP, and copy **dxgi.dll** plus the **complete ControlFGStreamline folder** beside **Control_DX12.exe**. Replace the previous Control FG files together when updating. Launch in **DX12** and press **F10** or your saved shortcut.

- **Steam:** Library → right-click Control → Manage → Browse local files.
- **Epic:** Library → Control's three-dot menu → Manage → folder icon beside Installation.
- **GOG Galaxy:** Control → menu beside Play → Manage installation → Show folder. Offline installations use your chosen game folder.

The separate **Control-FG-v2.1.1-Source.zip** requires compilation. See [INSTALL.md](INSTALL.md).

## GOG: turn off Galaxy's in-game overlay

Close Control. In Galaxy, select Control → menu beside Play → **Manage installation → Configure → Features**. Clear **Use default settings** if shown, then uncheck **Overlay / Access GOG GALAXY features in-game**. Save/confirm if prompted and relaunch the game.

Galaxy's overlay can cause input/focus stalls and make Control FG or other overlays disappear. If the per-game option is unavailable, **Settings → Game features → Overlay** disables it globally. Control FG's own F10 overlay remains available.

## Compatibility and known issues

Validated targets: **Steam 21225456**, **Epic 0.0.518.2177**, and **GOG EXE SHA-256 beginning 57A8912F**. DX11 and unvalidated game updates are not supported.

RR with **Ray Traced Indirect Diffuse Lighting** can still cause crawling/noisy streaks. Disable that game setting or RR as a workaround; the reflection clamp is not a fix for this artifact. The existing RTX 4070 overlay pacing report and HDR transition guidance remain in [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

A massive thank you to **HotKnives** for lending his machine and helping with QA for the recent releases and bug fixes!
