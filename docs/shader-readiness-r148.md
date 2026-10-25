# R148 — identify the unsupported baked-GI path

The supplied full R147 session confirms ready_mask=131072, all accepted corrected
shadow dependencies prepared, and two rejected baked-GI selections. Graphics and
pipeline-library hooks installed. Coverage reports 196 graphics and 238 compute
PSOs, zero same-size shader mismatches. No BakedGi variant was prepared. This is
not evidence of a missing installation file: the R53 target is a hash-locked
17,040-byte pixel shader captured with RT off. R147 incorrectly assumed it would
be available in the user's current RT path. Its synthetic GPU tests verified
switching mechanics, not the presence of that real shader in this configuration.

The session contains shader hashes, not bytecode. R148 adds a diagnostic-only CPU
snapshot of observed pixel and compute shader bytecode at initial identification,
including pipeline-library observations. At most 512 unique stage/hash pairs,
256 KiB per shader and 8 MiB total are retained. Creation uses a try-lock and
reports skipped observations rather than waiting; published copies are immutable.
There is no per-frame shader copying, GPU readback, shader modification or file
I/O in rendering callbacks. Save Log exports those copies from the diagnostic UI
thread into the existing log, with explicit start/end, sizes, offsets and hashes.
The decoder rejects truncated exports, missing/duplicate chunks and bad hashes.
This does not claim the unavailable modes now work or fix the shadow issue.

## Test

Install with Control closed. Keep the same game settings, load the affected room,
open F10, try Baked GI Off once, then click Save Log. Send the Lighting-R148 log
shown by the confirmation. No F9, walking sequence or video is needed for this
audit. It includes the shader evidence needed to identify the actual lighting
pass. Package-Lighting-Capture.cmd remains available for files over 200 MB.

The existing unsupported switch stays guarded. Default, exposure, accepted
rendering shaders, R146 capture scheduling and all shader sidecars are unchanged.
Raw records are observations, not proof every shader was bound or contributed.
Bound-pipeline hashes in the same log provide additional correlation. Dropped
observations and truncated identity logs must remain explicit limitations.

## Verification

Portable collector tests cover deduplication, capacity, concurrent publication,
failed writes/retry and immutable output. Decoder tests cover exact reconstruction,
SHA, truncation, missing/duplicate chunks and invalid metadata. Windows tests and
in-game snapshot confirmation are tracked separately in issue #15.
