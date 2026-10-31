# R253: native selection timing for wayfinding text

R253 adds observation BEFORE the request hook, to investigate the confirmed
Luck & Probability delay. R247 transfer behavior and the 4 ms / 16 pinned
partition settings remain unchanged. This build does not fix or accelerate mip
selection; it measures which native gating mechanism should be changed.

## Test

Install the complete package beside Control_DX12.exe and run
Start-CPU-R253-NativeGPUStorage-4ms.cmd. Load the area. Press F6 before approaching
the same three-sign board, then F6 when Luck & Probability becomes sharp. Stay
near the board for the 30-second observation window. Wait 40 seconds from the
first F6 and run Collect-Texture-Trace.cmd. Send its ZIP. No video or Reference
run is needed. The new selection data makes this different from repeating R252.

## Verified native path

Supplied Steam renderer SHA256:
EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433.
The read-only tools/audit-sign-selection.py checks the exact image and sites.

- Pool scan 168ED0 clears manager+80 budget time at 168F00. It scans entries
  starting at manager+88 with stride renderer+919010 (16 in the pinned launcher),
  then increments scan index at 169255 and cycle counter at 169263.
- The scan retains each candidate through its virtual +40 call, then releases
  it through +48 after processing. Call 16923E invokes selector 15A5E0 while
  that native ownership is held. The probe does not retain pointers for later reads.
- Selector computes level targets from distance/detail data, writes C4/C8 and
  delta C0, checks the global pending-byte count against 0x1900000 (25 MiB),
  and conditionally compares delta with global priority extrema 802C08/802C0C.
  It calls 15AB30 through 15AAAB or 15AAC2; both calls are observed.
- 15AB30 rejects a missing/non-idle level or an exhausted native budget unless
  bypass is active, then calls 158060 which allocates and issues the file request.
- The R252 Luck & Probability request-to-publication sample was 21.723 ms.
  This does not identify which earlier gate delayed it. Static code alone is
  not proof that any particular gate caused the user's visible delay.

All three observation patches require the existing exact-module hash gate plus
original E8 target validation. Partial installation rolls back. Original functions
are called exactly once, with their arguments/results and LastError preserved.
No native state is written by the observers. Existing call-patch fixtures and
new selector/budget forwarding fixtures exercise the wrappers on Windows.

## Records and coverage

R252's broad request capture remains: 16,384 rows, 30 seconds plus 10 seconds
completion grace. Its fields now use the R253 prefix. Selection names are learned
from texture requests throughout the session; only paths containing
\\props\\signs\\wayfinding_text_ enter the 64-identity registry. It includes the
confirmed sign and nearby wayfinding text. No dynamic name getter is called
inside the selector. Requests can invalidate or rename reused numeric identities;
unobserved object reuse remains a diagnostic correlation limitation. A target
not yet registered cannot have earlier selector observations. Inspect TARGET
records for Luck & Probability before interpreting absence of selection events.

TEXTURE_SELECT_R253_INSTALL reports successful installation. BEGIN/END delimit
the selector capture. TARGET maps integer texture identity to name. ROW gives
row index, texture, begin/end CPU microseconds, kind (0 selector, 1 budget gate),
requested native level for kind 1, and its boolean result. Two STATE records per
row (phase 0 before, 1 after) contain:

- scan index/cycle and pending bytes;
- raw B4/B8/C4/C8/C0 fields labeled base/resident/wanted/retain/delta;
- state of the wanted native level, enabled/special/deferred flags;
- priority min/max, policy/direction/bypass flags;
- accumulated native budget time, configured budget, and raw D4 distance metric.

Field labels describe reconstructed roles, not recovered source types. Native
streaming levels can group multiple mips. A budget-gate false result is not by
itself proof of budget exhaustion: inspect level state, budget and surrounding
selector data. A long gap between selector rows is not alone proof of scan delay:
earlier scan eligibility checks, missing registration and observation drops also
matter. Nested gate rows can precede the enclosing selector row in dump order;
use timestamps and row indices, not physical row order, for the timeline.

8,192 selection rows are buffered, then logged at the 40-second flush alongside
request rows. Capture overflow, target overflow and cumulative contention/drop
counts are explicit. Try-lock observers never wait for the diagnostic lock.
Zero timestamps/unmatched identities remain unknown, not zero delay. First
visible GPU use is still not directly measured; the second F6 marks the user's
observation. Snapshot/log flush adds diagnostic overhead. No FPS benefit is claimed.

## Validation

Local exact-image callsite/evidence audit; portable target capacity, row overflow,
reset and identity invalidation tests; Windows before/after-state, argument,
boolean return and LastError forwarding tests. Existing 72-mip GPU content,
resource lifetime, prewarm and graphics regressions remain enabled. The full-log
collector remains byte-preserving and tested with an open 5 MiB source file.
