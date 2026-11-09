# R228: consistent streaming partition comparison

This build corrects a diagnostic configuration problem; it does not claim an FPS gain or a stutter fix. Both launchers disable the R227 preparation phase hooks and retain the 8 ms request-admission budget. R226 Resolver remains disabled. Accepted visual fixes are unchanged.

## Run

Install over the previous experimental package and fully exit Control between runs.

1. Start-CPU-R228-Reference.cmd reproduces R227 Baseline behavior: applies 8 ms / 16 partitions at startup and mod-menu changes, but allows native graphics settings to overwrite partitions.
2. Start-CPU-R228-Pinned.cmd also preserves 16 partitions when native graphics settings are applied. Check for `STREAMING_R228_INSTALL ready=1` and `STREAMING_R228 pin_slices=16 writers=2`.
3. Compare the same stationary view and short traversal. Report stutter, FPS and texture arrival; collect the compact logs. No full video capture is necessary.

Pinning is process-local and opt-in. The normal game launch and saved menu defaults are unchanged. An explicit `CONTROLFG_STREAMING_PIN_SLICES=1` requires `CONTROLFG_STREAMING_SLICES=8` or `16`; invalid values do not install patches. The supplied comparison uses 16. If installation reports ready=0 in Pinned, the comparison is invalid.

## Why this build

The user's Native log showed `previous_budget_ms=8` and `previous_slices=2` after startup had applied 16. Static inspection found the exact native settings writer: renderer routine 0x1374B0 selects 2 partitions for source field +0x6B0 value 4, or 8 for values 0–3. The field's menu meaning is not yet established. The previous override only ran at startup and mod-menu changes, so its log described what was applied then, not guaranteed ongoing state.

R228 replaces only those two stores, with exact module/callsite checks and rollback. A tiny executable leaf writes the selected constant and returns, preserving registers and flags. There is no per-frame reapplication, settings-function wrapper, lock, callback, allocation or log in the native writer path. Windows executable fixtures cover both native instruction encodings, both permitted overrides, flags, R12, LastError, mismatched targets and rollback.

16 partitions visit fewer resource-pool entries per scan than 2 or 8. Keeping 16 may therefore change texture arrival or frame pacing. It is a controlled configuration experiment, not an assumption that 16 is the best value. The 8 ms request budget is not a hard limit on total streaming work or individual operations.

## RT preparation and DirectStorage

In the short stable Phases window, scene selection/assembly averaged about 3.224 ms of 3.298 ms total preparation. That bucket includes the initial spatial query. It does not establish that geometry packing alone dominates or that all of it can be cached. Persistent reuse still needs stable identities and correct invalidation for transforms, LOD, streaming, destruction and material remapping.

The user's standalone DirectStorage validation passed byte equality with GPU-optimized GDeflate support (0x19). DirectStorage remains a separate synthetic validator; the game does not load textures through it yet. No need to repeat that validator for this build. Native asset layout, decode requirements and completion/publication ownership remain prerequisites for integration.

The user reported stutter with the Phases launcher. Unequal workloads prevent attributing its cause to instrumentation alone; both R228 launchers leave those hooks absent.
