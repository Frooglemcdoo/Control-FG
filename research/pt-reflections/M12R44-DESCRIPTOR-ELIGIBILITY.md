# R43 result and R44 correction

The supplied R43 collector confirms the expected build, PT off, zero primary
preparation, and a real resolution change to 2560x1440. The user reports about
60 FPS in both DLAA and Quality. Final sampled counts are 216133 exclusive
operations, 9253 completed parallel operations, and one untracked operation.
Snapshot waiting is now zero, but the dominant copies still serialize.

R43 incorrectly reused an ever-interested filter to decide whether metadata
would change. A cached source token only describes a read; it need not be
invalidated by copying FROM that source. Historical UAV tags also survive after
an overwrite erased current UAV metadata. These false restrictions persist
across frames, sending ordinary copies back to the exclusive gate.

R44 separates conservative AS interest from the historical mirror filter. Under
shared ownership it checks the Bank's current UAV metadata and whether the
DESTINATION has a source token requiring invalidation. A source token does not
block the copy. UAV metadata either must be absent at both endpoints or have
the same format; otherwise the original exclusive transaction runs. Actual
descriptor contents and per-slot versions still update on every parallel copy.
All map reads occur under shared ownership, and all mutations remain exclusive.
AS markers are published for direct writes, single copies and bulk propagation.

Regression tests specifically cover cached sources, previously typed slots,
unchanged typed formats, fresh typed metadata and AS fallback. Existing tests
cover concurrent adjacent-slot writes, immutable snapshots and WARP readback.
No shader, scene ownership, PT toggle, or queue policy is changed. In-game
performance is pending: check PT-off DLAA/Quality first without WPR.
