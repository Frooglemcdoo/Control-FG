# R255: normal-launch storage integration

R254 is the accepted baseline: the user reported prompt texture upgrades with no noticeable frame spikes or stutter. Its final logged session completed all 10,937 accepted transfers, with zero failed or pending transfers. This is live observation and transfer health, not a GPU timing certification.

R255 enables the same native GPU storage and negative-delta upgrade gate by default when the verified Steam executable and renderer/D3D/runtime images match. Launch normally through Steam or Control_DX12.exe; no launcher or F6 is needed. Copy the full package, including dstorage.dll and dstoragecore.dll, beside the executable.

## Settings

- Texture Streaming Budget's former Native (stored value 0) choice is now Automatic. With the verified storage path installed it resolves to 4 ms. Otherwise it resolves to the native 1 ms budget.
- Existing explicit 4/6/8/10/12 ms choices remain respected. Select Automatic to use the accepted baseline. No INI migration or silent rewrite is performed.
- Successful storage installation enables 16 pinned scan partitions by default. Live menu budget changes respect the installed partition pin instead of temporarily writing a conflicting partition count.
- The partition count is a scheduling setting, not a VRAM pool size.
- Unvalidated Epic/GOG builds retain native storage and do not install the new default partition/gate patches. Existing graphics functionality is unchanged. Those storefronts still need their own native-storage verification before rollout.

## Diagnostics and overrides

Normal launching leaves CPU timing/workload hooks, storage latency tracking, publication observers, and F6 asset/selection observers disabled. Basic transfer counters, failure reporting and worker maximum-duration counters remain available. GPU resource lifetime, completion and ownership bookkeeping remain unchanged.

For requested diagnostics only, Start-R255-Storage-Diagnostics.cmd enables the observers and preserves the selected menu budget. F6 then records a 30-second window; collect the full log after 40 seconds using Collect-Texture-Trace.cmd. Normal acceptance testing requires neither F6 nor a special launcher.

Existing process-local overrides remain supported:

- CONTROLFG_NATIVE_GPU_STORAGE=0 disables native GPU storage and its default upgrade gate/pinning; Automatic then uses native settings.
- CONTROLFG_TEXTURE_UPGRADE_GATE=0 disables just the upgrade gate.
- CONTROLFG_STREAMING_PIN_SLICES=0 disables partition-writer pinning.
- CONTROLFG_STREAMING_SLICES=8 or 16 selects the diagnostic partition count. An installed pin wins over later menu changes.
- CONTROLFG_STREAMING_BUDGET_MS supports the existing exact numeric overrides.
- Diagnostic flags require an explicit 1. Malformed feature flags disable that feature; missing feature flags use the documented default.

Avoid old test launchers for normal testing because they explicitly enable diagnostics and override settings. Overrides affect this process; no environment variables are written by the mod.

## Validation

The build runs the existing 72-mip GPU transport/content tests, resource reuse/lifetime checks, executable gate forward/rollback test, observer forwarding tests, collector tests, and menu/schema checks. R255 adds policy tests covering missing/disabled/malformed environment flags, successful versus unavailable storage, saved budgets, and partition consistency.

Acceptance: launch normally with Texture Streaming Budget set to Automatic, revisit the same signs and traversal/combat route, and confirm R254 behavior remains. Logs should show DIRECTSTORAGE_R255 installed=1, TEXTURE_UPGRADE_R255 installed=1, and STORAGE_DEFAULTS_R255 active=1 automatic_budget_ms=4 pinned_slices=16 with diagnostic fields zero. STREAMING_R229_STATE reports effective budget/partitions when reporting is enabled.

No new compression, transfer scheduling, resource allocation, rendering or RT preparation algorithm is introduced. RT preparation measurement is the next independent investigation after this integration passes live testing.
