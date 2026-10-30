# R236: resolved archive candidates and loader stages

R235 live validation succeeded. In the latest session, reports after present 1440 included 3,315 matched requests: construction-to-dispatch averaged 15.38 ms and whole worker execution averaged 1.38 ms. Report-interval queue averages were 4–22 ms. Whole-run maximum worker execution was 171.586 ms. These are request durations, not additive frame costs or predicted savings. One contention drop and 479 replaced tracker slots occurred. Streaming remained pinned at 8 ms / 16 partitions.

## Architectural finding

The new selector at runtime 0xFC5E0 already batches newly pending records through PackFileSystem::getFileLocationInfo at 0xFC853 -> 0x18F760. The pending-list lock has been released at this point, but the selection-list lock at loader+0x518 is still held. We must not add file opens, DirectStorage submission, waiting or unbounded work inside that lock.

The lookup writes PackFile identity at record+0x120, a sorting location at +0x128 and file size at +0x130. The sorting location is entry+0x20 PLUS pack+0x288. PackFile::readFile instead reads from entry+0x20 plus the requested partial offset. Passing the sorting location directly to DirectStorage would therefore address the wrong bytes when the bias is nonzero.

R236 observes the existing lookup without calling it again. For at most 256 records per batch, it subtracts the pack bias, checks partial offset/size against file size and overflow, and copies scalar identity/range/timestamp into a bounded table. Sentinel full-file requests and unresolved/invalid ranges are excluded. No filenames, listeners, destinations, handles or dereferenceable native pointers are retained. Numeric pack identity is used only for equality. The table is try-lock only; collisions and contention are reported rather than blocking.

At native dispatch, exact loader identity/request ID consumes a candidate once. At the existing PackFile read call 0x18E4F5, the observer compares its predicted raw archive range with the actual entry offset, requested offset, length and numeric pack identity. Native reads still execute unchanged. `resolved_lead` measures time from candidate recording AFTER the native batch lookup to worker dispatch, not the larger admission-to-dispatch interval. Candidates from large batches are explicitly incomplete (`capped`). Old-selector runs can produce no candidates; selection counters distinguish that from failed instrumentation.

## Stage timings

All loader worker dispatches establish a scoped thread-local observer, independent of whether R235's partial-request table matched. Nested scopes restore correctly. The patch set observes:

- `readfile`: actual native synchronous Win32 ReadFile at 0x1933F1; completed bytes and failures counted. This measures the API duration, including any OS-cache service; it is not physical-disk-only time.
- `pack_read`: complete PackFile::readFile including seek and possible initial archive open. This INCLUDES `readfile`; do not sum them.
- `buffer_wait`: two existing SleepConditionVariableSRW calls at 0xFB6B4 / 0xFB80A, used while loader buffers are unavailable. Other allocation and lock costs remain outside this metric.
- `interleave`: native waitForInterleaveClients call at 0xFB8F3.
- `lookup`: the original native batch location lookup only.
- `resolved_lead`: candidate recording to dispatch, a lead-time metric, not CPU work.

Counts, total milliseconds and maxima are emitted every five seconds. Maxima can belong to different requests; never subtract maxima to assign a spike. Stage totals use a broader scope than R235 matched-worker totals and cannot simply be subtracted from those totals. No per-request disk logging, native queue reordering or synthetic completion occurs.

## Cancellation audit and next integration boundary

cancelRequest(ID), 0xFB1A0, takes the selection and pending locks, searches both queued lists using +0x178, swaps the last item over a removed entry if needed, destroys the last record and decrements pending count +0x528. It returns false when neither queued list contains the ID. This code does not establish an in-flight cancellation/completion contract. A future detached I/O buffer must have independent ownership and retirement even when queued work disappears. R236 creates no detached I/O and does not change native cancellation, publication, decode or buffer lifetime. Stale diagnostic entries are bounded and may be replaced; replacement is not a cancellation count.

Before early DirectStorage submission: confirm resolved lead and range equality in the live workload, design bounded handoff outside the selection lock, hold an independently validated archive file identity, and ensure abandoned I/O retires without touching game buffers. This build is diagnostic groundwork, not the streaming replacement or a speedup claim.

## Run

Copy the complete package beside Control_DX12.exe and run Start-CPU-R236-LoaderStages.cmd. One pinned traversal is sufficient; send compact logs with texture/stutter observations. No Reference or standalone validator run is required. The launcher preserves 8 ms / 16 pinned partitions, disables expensive preparation-phase hooks and keeps the demand-time cache OFF.

Expect LOADER_R236 installed=1 plus LOADER_R236_CANDIDATES / LOADER_R236_STAGE and the retained LOADER_R235_QUEUE reports. DIRECTSTORAGE_R230_INSTALL ready=0 remains intentional. Enabling the old DirectStorage cache and this trace together is rejected because both would patch the same native read site.

## Validation

The supplied Steam runtime SHA and all direct targets/indirect import bytes are checked by tools/audit-native-loader.py. Startup installation prepares all ten direct calls and three import-call patches before mutation, and rolls back applied patches on failure. Portable tests cover bias subtraction, bounds, overflow and bounded candidate matching. Windows forwarding fixtures cover five-argument native reads, results, LastError, candidate equality/mismatch, nested scopes and batch caps. Live range comparison and stage distribution remain to be verified in the game.
