# R151 — RR input/output boundary comparison

R150 completed six paired direct-light samples with zero failures and 24,576 valid records. Nearby receiver comparisons did not demonstrate a broad direct-pass collapse, but do not rule out local shadow loss. R149 baked-GI removal darkened the game without fixing the reported issue and is not a candidate fix.

R151 packages the existing guarded RR input/output capture with a current-schema collector. It requests no new rendering alteration. Ctrl+Alt+F9 captures two evaluated frames; repeat at the second position for four frames total. Native input color, RR output, depth, motion, guides and raw GI are retained with their actual formats, dimensions and frame metadata. This does not capture final tone mapping or prove the same receiver across positions. The video supplies visual context; it is not synchronized pixel-for-pixel with these raw frames.

## Test

- Install with Control closed, then start a fresh game session with RR on and Default selected.
- Run Capture-RR-Boundary.cmd beside the game. Return to gameplay and close F10.
- Where the shadow is visible, press Ctrl+Alt+F9 once. Hold still until the completion beep.
- Move to where it fades, press Ctrl+Alt+F9 again and hold still until the second beep. Record video showing both positions; keep graphics settings unchanged.
- The helper copies the two pairs and current-session logs into a Desktop folder, then packages them below 200 MB per ZIP. Send all ZIPs in its printed SEND folder plus the video. No manual folder hunting or GI-off comparison.

The helper has a 180-second deadline per pair. Missing output, failed evaluation, nonconsecutive frames or invalid resource sizes are reported as incomplete, with diagnostic logs packaged. Original captures remain available. Start a new game session if the two allowed capture requests have already been used. This helper requires Python 3 and uses Windows PowerShell 5.1-compatible launch code.

## Validation

Collector tests verify current-session/ordinal separation, freshness, two consecutive frames, successful RR output and exact raw-file sizes. Existing native resource-state/format guards, original-list copying, state restoration and fenced worker export remain unchanged. Windows shader, proxy, menu and capture tests still run. No GPU capture implementation, accepted shader fix, RR guide behavior, baked lighting or exposure policy changes in R151. In-game collection and the underlying shadow-loss fix remain unverified until the next result.
