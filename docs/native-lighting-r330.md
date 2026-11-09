# R330: reduce Owned GI CPU preparation overhead

Changes derived from the R329 tracked Native and Owned captures:
- Defer the duplicate catalog traversal check to scene preparation at the same protected submission cut. Preflight and other callers retain full catalog validation.
- Repeated identical mappings reuse already-validated geometry metadata and avoid repeated token-set insertions. Every instance transform and row conflict is still checked.
- Carry resource/heap leases and immutable buffer descriptions into table commit instead of rereading resource private data. Retain range/alias and geometry validation.
- Maintain per-allocation wrapper identities for direct lookup, including wrapper retirement and shared resources.

No shaders, ray counts, lighting quality, or GPU geometry-copy freshness were changed. The GPU admission gate remains necessary for correctness. The changes reduce work before that gate is released; actual frame-time savings require an in-game capture.

Install over R329 preserving folders. Use the same Owned launcher and F10 RECORD GI button. Five seconds, one beep still, next beep slow pan, two beeps close. Packaging starts after normal exit.
