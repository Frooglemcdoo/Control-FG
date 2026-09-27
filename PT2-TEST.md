# PT2 — Toggle, GPU Counters, Late Visible Composite

PT2 keeps PT1 R2's proven Control TLAS recovery but stops writing the diagnostic into Control's reflection buffer.

## F10 overlay
A new PATH TRACING section provides:
- PT OFF / PT ON
- HIT/MISS
- DISTANCE
- INSTANCE

PT OFF is the true native comparison baseline: no PT ray dispatch and no PT composite.

## PT ON
PT2 traces the custom DXR 1.1 RayQuery pass into a private FP16 texture. It also records sampled GPU counters every 16 pixels in X/Y:
- sampled rays
- hits
- misses
- min/max/average hit distance
- instance checksum

The private result is composited at the presentation boundary so Control/RR cannot overwrite it:
- SDR: after the existing SDR correction and before real Present
- HDR: into Control's FP16 shadow backbuffer before HDR10 conversion

F7 cycles the three debug views only while PT is ON. F8 remains the PT0 detailed-capture hotkey.

## Expected visual proof
Enable PT and select HIT/MISS. The whole scene presentation should become a stark black/white custom ray visualization. Turning PT OFF should immediately restore native Control rendering.

Key markers:
- PT2_TOGGLE
- PT2_READY
- PT2_DISPATCH_OK
- PT2_COMPOSITE_READY
- PT2_COMPOSITE_OK
- PT2_COUNTERS
