# R152 — Overlay RR capture

R151 only beeped after both frames were validated and copied. The user heard no beep; that does not establish whether shortcut detection, capture admission, export or sound failed. The user requires an overlay button, not capture hotkeys.

R152 adds CAPTURE RR and PACKAGE RR buttons using the existing overlay fonts and button layout. The capture button publishes one atomic request; the renderer consumes it under the existing capture lock. Diagnostic-build capture no longer polls Ctrl+Alt+F9. The panel remains visible with gameplay focus and shows requested, preparing, capturing, saving, saved or failed plus the saved-pair count. Known native-input/GI admission rejections appear in the status. A failed export or missing RR output cannot count as a successful pair. Duplicate clicks and requests past two completed pairs reject.

PACKAGE RR starts the collector for this exact process, collects already completed pairs without waiting for another shortcut, retains incomplete metadata and logs, and opens the resulting SEND folder. Missing frames are explicitly marked incomplete. The console remains open on completion/failure. All ZIPs remain below 200 MB and original captures are retained. This uses the installed Python helper but no separately launched command is required.

## Test

1. Install with Control closed, then start a fresh session with RR on and Default selected.
2. Where the shadow is visible, open F10 and click CAPTURE RR. Keep the camera still until the panel reads Saved pairs: 1/2.
3. Move to where the shadow fades, click CAPTURE RR again and wait for Saved pairs: 2/2.
4. Click PACKAGE RR and send all ZIPs from the opened folder plus video of both positions. If the status fails or stays waiting, use PACKAGE RR to send diagnostics instead.

The same guarded RR texture/state validation, original-list copies and fenced worker exports are used. No change to rendering shaders, exposure or RR guides. Unrelated hair/material shader audits are not armed by this focused diagnostic button. A failed GPU recording is not recycled. The existing two-pair session limit remains.

Portable request-state tests cover pending publication, duplicate requests, partial work, successful outputs, failed exports and the pair limit. Collector tests cover provenance, complete raw files, incomplete metadata and bounded status-log reading. Windows CI also validates PowerShell parsing and builds the overlay. Gameplay operation remains to be checked; the cause of R151 silence and the underlying shadow-loss issue are not yet proven.
