Control-FG Primary PT M12R40 - repeated single-descriptor copies

R39 recorded 10.9 million single mirror copies and ZERO bulk mirror batches.
The bulk-copy optimization did not address the exercised game path. R40
caches the tracked source generation for each mirrored descriptor slot.
A repeated copy of the same unchanged source skips the private mirror copy
and format-map work. The original game copy always executes unchanged.
Source writes/copies, direct destination writes, CPU heap creation/reuse,
snapshot refresh, bulk copies and invalidation revoke cached proof.
The bounded source cache never reuses generations after eviction.

Close Control; install all files beside Control_DX12.exe. Keep the startup
baseline bypass disabled (PT-Capture-Restore.cmd removes its marker).
Test DLAA and Quality in one stationary scene with FG off, first PT off,
then PT on, 15 seconds each. Toggle PT off and verify recovery.
Run Collect-PT-Primary-M12R40.cmd and upload the results ZIP.

PT_PRIMARY_MIRROR_COPY now includes redundant_skipped. Compare actual copies
and sampled descriptor timings. Windows/WARP tests validate descriptor
contents and invalidation, not in-game FPS. Lighting remains unchanged.
