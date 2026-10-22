# R100: preserve the active native GI filter with RR

R99 confirmed 10,154 successful RR evaluation returns, matching native GI bypass observations and reflection clamp calls. Both broad diffuse call sites remained unused. R98 targeted the wrong path for this room.

## Test

Install beside Control_DX12.exe with the game closed. Keep RR ON, native RT Contact Shadows and indirect diffuse lighting enabled, and FG/glass enhancements OFF. At the same projector-room camera compare CENTER FILTERED with GI FILTER, waiting 10 seconds in each. Switch back once to confirm repeatability, then SAVE LOG. Send the Lighting-R100 log and comparison images. No restart between modes.

CENTER FILTERED retains the R97 shadow baseline and existing RR GI bypass. GI FILTER keeps those same shadow shaders and restores the native GI temporal/spatial filter branch. The earlier DIFFUSE FILTER button still targets the inactive broad path; do not use it for this comparison.

## Implementation and limits

Recovered renderer disassembly confirms the option call at +0x12cd45 chooses either history release at +0x12cd55 or native GI filtering at +0x12cd96 (target +0x14c90). The filtered branch reads the native pass count from the option at +0x913400 and supplies its own color and history arguments. Runtime installation verifies the original call target before patching; failed installation rolls back.

The mode is latched at the start of each RR frame. Only full RR with indirect diffuse lighting and a positive native pass count selects filtering. If the pass count is zero/negative, the existing bypass remains active and `gi_filter_unavailable` increments. No pass count or game option is forcibly changed. Switching signal policy requests an RR history reset. Native GI history creation, reuse and release stay under the engine's existing control.

A new hook observes the native GI filter return. RR readiness requires the correct history object, retained history, non-null color, positive passes, matching frame and Lighting stage. Bypass evidence cannot satisfy a filtered frame and vice versa. Original arguments, exceptions and LastError are preserved. No fake GI completion is recorded at the option getter.

The single log retains R99's counters and adds `gi_filter_entry`, `gi_filter_validated` and `gi_filter_unavailable`. An increase in `gi_filter_validated` together with `rr_success` establishes that this test ran with successful RR API returns; it does not establish GPU image quality. A zero count is not a successful restoration test.

This is a diagnostic prefiltering comparison, not yet a proven correction. All 14 shadow shader assets, the 60% reflection clamp, RR guides/skin and glass source remain unchanged. No custom GPU waits, readbacks or render-thread file writes are introduced. Native filtering has its normal GPU/resource cost.

Validation compiles the actual production GI hook with instrumented native-call mocks, covering argument forwarding, return order, history/frame/stage/pass mismatches, duplicate evidence, exceptions and LastError. Existing WARP checks cover centered ray/selector/filter routing; actual game appearance still requires this test.
