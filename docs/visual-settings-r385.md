# R385 Visual settings integration

R384 Owned GI + RELAX remains the user-accepted rendering baseline. R385 moves feature preferences into the existing native Visual menu without changing RELAX shader parameters.

Enabling RR restores Owned GI, RELAX, four indirect samples, the Native reflection sample policy and clamp60. These are automatic parts of RR, not separate Visual controls. The old saved GI sample override is no longer loaded or saved. RR itself retains its existing saved On/Off choice.

| Visual section | Control | Default |
| --- | --- | --- |
| Display / Monitor | FPS Counter | Off |
| Quality | Enhanced Character Lighting | On |
| Ray Tracing | Enhanced Lighting and Shadows | On |

The controls use native checkboxes, acknowledged updates, INI persistence and reset defaults. Ordering inserts after the matching native VSync, volumetric-lighting and contact-shadow rows when available; otherwise each stays within its correct existing section. Character lighting selects native or the stable rig/near-volume/diffuse reconstruction combination. Enhanced Lighting selects Native or Native Blend with projection rays and Map Guard off. Lighting selection is consumed by the renderer when its shader path is available.

F10 removes the migrated character, FPS and Native Blend feature controls, plus GI sample, specular sample and GI-filter controls. Captures and advanced comparisons remain. GI RAW is an explicit temporary comparison. RESET COMPARE clears manual GI/contact overrides and returns GI to the automatic RR denoiser; it does not overwrite saved Visual settings. Native GI remains an advanced comparison. Re-enabling RR restores the bundled defaults.

Validation includes native-menu row placement and fallback ordering, checkbox acknowledgements, invalid values, readback/reset, catalog storage mappings, generated-schema consistency and clean-baseline checks. Full Windows build and live menu validation are separate gates. The test build requires checking both main-menu and pause-menu behavior, save/restart, RR off/on and visual parity with R384.
