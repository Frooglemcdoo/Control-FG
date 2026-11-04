# R331: repeated-row admission and dispatch-gate allocation

R330 captures confirmed lower preservation and commit costs but approximately 7.7-7.9 ms scene mapping remained. This revision:
- Directly reuses a previously admitted binding row when its exact BLAS address/token/revision/geometry matches, within one owner-locked admission. Instance transforms, scene closure, and conflicting identities remain checked.
- Carries the exact TLAS and BLAS records from traversal validation into mapping, eliminating repeat history selection. Nothing crosses submission cuts.
- Pools the two small dispatch-gate buffers only after the enclosing recording/flight releases them. Live gates never share a flag, upload zero is rewritten, and prefix arm/replay-close semantics remain unchanged.
- Reports mapping traversal, native table verification, witness indexing, and remaining row work separately in the existing capture logs.

No shader, ray-count, or lighting changes. No measured in-game performance claim. Install over R330 preserving folders; use the existing Owned launcher and F10 RECORD GI button.
