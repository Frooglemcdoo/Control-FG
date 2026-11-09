# Settings and UI architecture

This refactor covers the native settings menu and its supporting service. The
renderer, FG scheduling, RR guides and glass shaders retain their existing
implementation. It is not a claim that the entire older renderer has been refactored.

## Ownership

| File | Responsibility |
| --- | --- |
| `src/settings_schema.json` | Stable IDs, names, descriptions, types, defaults, choices, ranges, dependencies, visibility and INI keys |
| `src/settings_schema.generated.h` | Generated C++ metadata; never edit manually |
| `src/settings_catalog.h` | Named renderer adapters and shared validation, defaults, snapshots and stored-value conversion |
| `src/fg_settings.h` | Startup loading, compatibility with existing INI files, debounced saving and service lifecycle |
| `src/native_menu/menu.js` | Generic native-control row creation, dependencies and acknowledged UI state |
| `src/native_menu/settings_bridge.h` | Coherent event transport and shared catalog application/reset/readback |
| `src/native_menu/runtime.h` | Version-gated game integration, vtable installation and resource ownership |
| `src/native_menu/bundle.h` | Exact guarded anchors in the supported game bundle |
| `src/ui/theme.h` | Shared font families/styles, colors and monitor layout tokens |
| `src/ui/vram_monitor.h` | Read-only monitor drawing; no persistence or input handling |

Native menu controls inherit Control's fonts and style. Do not duplicate those
fonts in each option or override the game's CSS. Custom Win32 drawing uses named
theme tokens. Change a font family once in `theme.h`, not at each drawing call.

## Adding an option

1. Add a named entry in `settings_schema.json`. Choose an unused stable ID inside
   the reserved range, excluding the handshake ID. Supply readable labels and
   descriptions, a type, default and choices or integer range. Add an INI mapping
   only if this option owns storage; derived controls must not duplicate keys.
2. Add the corresponding named `Key` cases to `Read` and `Write` in
   `settings_catalog.h`. Keep renderer side effects here. Return false if the
   change fails so the generic UI acknowledgement restores the confirmed value.
3. Add startup application in `FGSettingsLoadSettings` if the setting persists.
   Use `FGSettingsReadStored(Key::YourOption)` for the catalog's key/default.
   Startup remains separate from live application deliberately: GPU mutations
   must not run before the renderer is initialized. Experimental MFG bootstrap
   still reads its existing startup key before this service starts.
4. Run `python tools/generate-settings-schema.py` and
   `python tools/embed-native-menu.py`. Commit both generated outputs.
5. Extend behavior tests for the new adapter or dependency. No edits to generic
   row rendering, reset iteration, save iteration or snapshot lists are needed.

Dependencies refer to named option keys, not array positions. Current capability
policies are `frameGeneration` (choice filtering) and `rtx40` (visibility); adding
a new policy requires both native validation and UI support. Definitions are
ordered for display and wire snapshot slots; both sides consume the same schema.
Do not reuse IDs or change persisted INI key names to reorder the display.

## State and persistence rules

- The catalog is the shared definition, and renderer getters are runtime truth.
- Both initial configuration and live readback use `Snapshot`.
- `Write` validates values before applying changes. Successful callers mark
  settings dirty; the service saves after the existing debounce or on native save.
- Save iterates catalog storage entries, preserving the existing INI format.
- Reset iterates catalog defaults. Its explicit write context prevents resetting
  the target slider from accidentally switching Dynamic targeting to Manual.
- Dynamic target FPS stores zero for Automatic. The slider separately remembers
  the last manual value during the session; `ReadStored` owns this conversion.
- The native bridge holds `fgSettingsMutex` while accessing the catalog. Startup
  loading occurs before hooks/service exposure. Renderer-owned atomics remain
  responsible for render-thread consumption.
- Unsupported game versions never activate ABI hooks. They keep saved settings
  and log the mismatch; there is no legacy popup fallback.

## Verification

`generate-settings-schema.py --check` rejects stale C++ metadata, duplicate IDs,
duplicate storage ownership, invalid defaults, and out-of-range protocol values.
CI also verifies that embedding the JS/schema produces the committed header.
Portable tests cover validation, unsupported choices, failed writes, reset,
Automatic/Manual target semantics, live readback and native event forwarding.
A cross-language test compares emitted C++ defaults/choices with the JSON schema.
The exact supplied game bundle is tested locally without committing game assets.
Windows CI compiles the DLL and runs the existing renderer/guide/glass gates.

## R44 review corrections and remaining boundaries

Review found and corrected saved FG Off being forced to 2x on RTX 40; an
unsupported 2x reset fallback; UI-only dependency enforcement; stale capability
choices/reset defaults after resource load; missing slider-step validation; and
a final INI flush failure being reported as successful. Dependency requirements
now generate into C++ and are enforced for user writes. Reset intentionally
bypasses dependency enablement while retaining range and GPU-policy validation.
Live readback refreshes FG choices and all reset defaults before sending values.

Catalog validation now rejects dependency cycles, nonexistent dependency values,
invalid step alignment, duplicate localization keys, case-insensitive duplicate
INI ownership, unknown policies and unsupported bit widths. CI exercises rejected
catalog edits as well as native callback and UI behavior.

Remaining boundaries: this integration supports the verified game DLL/UI build
only. Real controller navigation, overlay coexistence, GPU capability transitions
and disk-write failures still require Windows/in-game validation. Startup renderer
application is deliberately explicit; a new persistent setting still needs its
startup adapter. This review does not certify the older rendering subsystem.

## Choice sliders (R45)

Set `presentation: "slider"` on a choice option to reuse the game's native integer
slider. Positions are indices into its choices, with explicit labels; UI events
map the index back to the real stored value before entering the native bridge.
This avoids invalid intermediate budgets/counts. FG retains the native dropdown.

`nativeRequires` lists native option localization IDs. The menu observes the
Graphics model and disables a row when its native prerequisite is off or missing.
GI samples require RR plus `MENU_OPTIONS_RT_INDIRECT_DIFFUSE_QUALITY`. The audited
producer uses `rt:Sample Count Diffuse` and `g_uRTDiffuseRayCount`; Contact Shadows
is a separate pass. While inactive the slider displays Native without overwriting
the saved requested sample count. Actual override remains gated by full RR and
execution of the native diffuse producer.

## Native group placement and fixed RR policy (R46)

The schema's `layout` names native group anchors and header boundaries. Each
option declares its group; the generic ordering pass appends it within that
group while preserving native row order and stable IDs. Monitor holds FG controls,
Quality holds Texture Streaming, and Ray Tracing holds RR, GI and glass.

`presentation: "checkbox"` uses the native Toggle component. Custom boolean
events are normalized to 0/1 and sent through the existing verified enum bridge;
original game boolean events pass through untouched. No additional ABI hook is
needed. The model and reflection-detail settings are removed from the catalog,
INI reads/writes and dispatch. Model F and 60% are renderer constants; there is
no setter or selectable Model E. Only the 60% clamp shader is compiled/cached.
The retired numeric IDs are unassigned and rejected by dispatch.
