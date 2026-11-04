# R320 — identify geometry rejection across the selected scene

R319 capture (PID 18952) verified the deferred queue hold releases and scene selection advances to private_scene_table/detail=3. Zero owned passes were admitted. Existing binding/table snapshots do not include the selected BLAS witnesses and live/frozen allocation mappings needed to disambiguate that error. Repeating the same RR capture would not supply those fields.

R320 preserves strict admission and R319 synchronization. It records a full CPU geometry census for the first three Prepare attempts in the process, counts every geometry failure by stage, and retains up to four detailed examples per stage. Subsequent calls fail fast as before; no GPU readbacks or extra geometry copies are added. Failed transactions never publish partial tables or arm dispatch controls.

Stages: witness identity, transform, missing witness, metadata agreement, exact allocation/binding resolution, native table conversion, frozen-range rebase, conflicting table row. Samples include instance/geometry/row, exact BLAS address/token/revision, all native binding constants, geometry witness, current allocation addresses/sizes/tokens, frozen source/destination ranges, resolver and rebase sub-errors. Counters cover the full selected scene rather than a screen crop. Structural closure/mapping errors still reject immediately.

Resolver mismatch bits: 1 addresses, 2 geometry shape, 4 captured tokens, 8 binding index token, 16 binding index stride, 32 index span, 64 position ownership. Bits are ORed over candidates in the matching index bucket, not a claim that one candidate failed every bit.

Rebase stages: 0 none, 1 owner, 2 original admission, 3 converted record, 4 index range, 5 position range, 6 private admission. Admission enum: none, format, epoch, allocation, alignment, range, material. Resolver status: candidate, missingAllocation, missingGeometry, ambiguousGeometry, invalidBinding, invalidGeometry.

Validation extends the existing 16/32-bit scene transaction test with frozen-index-range attribution and two simultaneous independent geometry failures. It verifies both are reported and the previously valid table remains unchanged. Existing queue gate and ownership tests remain required in CI.

Run Start-GI-Owned-Test.cmd, RR and RT indirect lighting on, GI RAW BYPASS. No capture hotkey or standalone producer test is needed. Exit normally for the diagnostic ZIP. OWNED WAIT remains expected until the measured geometry cause is corrected; this is not a claimed lighting or FPS fix.
