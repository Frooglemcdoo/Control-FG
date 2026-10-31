# R256: native DirectStorage checkbox

Copy the full package beside Control_DX12.exe, including ControlFG-Restart.exe, dstorage.dll and dstoragecore.dll. Launch normally. No F6 or test launcher is needed.

## Menu changes

- Graphics > Quality > DirectStorage now uses the game's original Toggle component, identical to SSAO and other native checkboxes. No custom layout, font, checkbox artwork, sizing or CSS is substituted.
- The existing Toggle's EventButton receives confirmation properties only for DirectStorage. Other native checkboxes keep their original behaviour.
- Selecting either On or Off shows the same native restart confirmation as R255. Cancel leaves the setting unchanged; Confirm saves it, requests normal exit and relaunches after the old process closes.
- Texture Streaming Budget is removed from the visible menu. Its old saved selections are ignored on startup. DirectStorage uses the accepted internal 4 ms / 16 pinned partitions; disabled or unavailable storage uses the native automatic budget/partitions. Explicit diagnostic environment overrides remain available.
- Reset Graphics does not toggle DirectStorage or restart the game.
- The checkbox is disabled on unvalidated builds. Native storage support remains verified-Steam-only.

## Preserved behaviour

R254 transport, worker prewarming, negative-delta texture-upgrade eligibility, resource ownership and native completion behaviour are unchanged. R255 saved DirectStorage preference, restart helper and disabled-by-default observers are retained. The helper preserves the executable, command line and working directory, clears old storage overrides, waits for parent exit, and never terminates the game. Missing-helper/preparation/save failures reject the restart.

## Validation and acceptance

Local checks patch the supplied original commons.bundle.js and execute its native OptionsTab and Toggle renderers. They verify DirectStorage and SSAO share the same component, layout classes and check symbol; only DirectStorage receives confirmation properties. The hidden budget has no visible row and its UI events are rejected. Confirmation routing, reset preservation and bundle identity/relocation tests pass locally.

The Windows build also runs the existing restart-helper process tests, GPU transport and resource-lifetime tests, executable upgrade-gate test, and native-menu tests.

Live acceptance: confirm the slider is gone and DirectStorage matches nearby checkboxes. Test Cancel, then Off/restart and On/restart. Revisit the accepted traversal/sign route with DirectStorage on. Actual game rendering, controller navigation and Steam relaunch still require this live check.
