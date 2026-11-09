# Native Graphics menu integration investigation

Status: R41 implementation with offline tests; live game validation pending.
Based on the supplied Control DX12 executable, coregame/UI DLLs, and the
2026-10-20 menu collector output. R40's glass-only runtime remains unchanged.

## Confirmed UI path

`menu.ui` embeds menu.html, menu.bundle.js, and menu.css. `persistent.ui`
embeds commons.bundle.js, commons.css, and coherent.js. These are uncompressed
PC version-2 serialized resources. `tools/read-native-ui.py` validates the
metadata and ranges and can extract their contents into a new local directory.
Do not commit or distribute extracted proprietary assets.

OptionsMenu passes `g_graphicsOptions` to the shared OptionsTab component with
sectionType `Graphics`. OptionsTab combines the typed property arrays, sorts
them by `m_iPropertyID`, and renders the game's existing dropdowns, toggles,
and sliders. Both pause and main menus use this route. This permits adding
real native rows without recreating their mouse/controller navigation.

The controls trigger `On${valueDataType}${section}PropertySet` with the property
ID and selected value. Thus enum changes use `OnEnumGraphicsPropertySet`.
Back navigation emits `OnSaveOptions`; the reset button emits `OnGraphicsReset`.

Labels are localization IDs, not literal display strings. Text.tsx marks
untranslated strings with `unlocalized_text`, styled hotpink in commons.css.
A label/description integration is required; injecting literal labels alone
does not produce a finished menu.

## Native interface evidence

RVAs below apply only to the supplied coregame DLL. They are investigation
notes, not permission to install unchecked hooks on other builds.

| Interface | RVA / offset |
| --- | --- |
| UIGraphicsOptionsModel vtable | 0x5fb858 |
| initProperties | 0x1cb490; vtable +0x30 |
| onEnumPropertySet | 0x1c9940; vtable +0x58 |
| onReset | 0x1ceb60; vtable +0x28 |
| onSave (base implementation) | 0x1d5b60; vtable +0x18 |
| updateModel | 0x1ce830; vtable +0x78 |
| Enum property vector | model +0xb0 |
| Vector data / count / capacity flags | +0 / +8 / +12 |
| Enum entry stride | 0xb8 |
| Enum entry type / ID / current value | +0x40 / +0x44 / +0x48 |
| Enum entry callback object pointer | +0x98 |
| Enum entry possible-value vector | +0xa0 |
| Native Coherent view | model +0xd0 |

The native enum callback updates its property, invokes the property's callable,
notifies the options integration, and updates renderer settings. Custom IDs
must be handled before forwarding to the native renderer path. A null callable
is not safe if the normal property setter reaches that entry.

Native enum construction and relocation involve engine strings, allocator-owned
vectors, and MSVC callable storage. Raw memcpy of a native entry is unsafe:
in-place strings and callable storage contain self-relative pointers.

Graphics reset calls the integration reset, resets native renderer settings,
then invokes initProperties and updateModel. It is not the same operation as
the base `reset` method at vtable +0x38, which clears property storage during
reinitialization. Mod defaults must be reset only on the user reset event.

## R41 implementation

The runtime patches the shared OptionsTab bundle in memory as PageResource is
serialized, before Resource::load publishes it. It uses the engine byte-vector
reserve operation and updates embedded file ranges; disk archives are untouched.
It adds observable JS row data, using the original renderers and descriptions.
It does not construct or append ABI-sensitive native property objects.

The extension uses reserved enum/int IDs 10000–10010 and handshake 10999.
UIGraphicsOptionsModel vtable hooks intercept those IDs before native renderer
handling. Other IDs pass through. Native acknowledgements use the verified
no-argument View::TriggerEvent slot +0x218; rejected changes restore the last
confirmed displayed value. Reset applies mod defaults and preserves native reset.
The native save event flushes existing INI persistence; the existing timer retries
failed writes. A recursive mutex serializes settings writes across UI threads.

The DLL and UI implementation are version gated. The exact commons bundle hash
and unique replacement anchors must match before mutation. Unsupported builds
keep saved settings without a popup fallback. R42 removes the old settings
window and its keyboard polling, mouse capture and unused HDR replay hooks.
Glass remains available; opaque reflections and all opaque runtime hooks remain
absent. The shaders and RR skin/hair guide code are unchanged.

Offline checks cover the supplied bundle, native component rendering, translated
labels/descriptions, capability filtering, value validation, acknowledgements,
reset, callback forwarding, and allocator-owned buffer relocation. CI builds the
Windows DLL and runs portable bridge tests without redistributing game assets.

Still requires Control: visual/navigation verification in main and pause menus,
actual Coherent callback delivery, and persistence after closing/restarting.

## R42 menu-only cleanup

`fg_settings.h` retains persistence, GPU policy enforcement, display-refresh
discovery and the optional read-only VRAM monitor. The 1,589-line popup module
is removed. The current CI build no longer embeds its logo resource or links
its bitmap compositing library. Historical build fixtures remain provenance.

The VRAM monitor is an owned, non-topmost, nonactivating, transparent window;
position updates occur only when placement changes and preserve z-order. This
removes repeated global topmost promotion, a possible overlay interference
source. NVIDIA overlay operation still requires in-game verification. No input
hooks or shortcut registration are installed by the settings service.

Each custom InventoryHeader is enclosed in a stacking context at z-index 0.
Its native text z-index of 999 can no longer escape over dropdowns (z-index 5)
or the native dimming layer. Fonts, spacing and dropdown controls remain native.

Descriptions now explain effect, dependencies and performance/memory tradeoffs.
Live settings readback on view creation remains in place from the R41 fix.
