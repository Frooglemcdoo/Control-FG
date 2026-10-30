# R235: upstream loader trace, demand-time cache disabled

R234 proved that shrinking demand-time read-ahead did not solve the timing problem: 1,557 hits from 19,582 reads, 16,963,101 served bytes, 2,553,311,240 submitted bytes; 2,467,987,848 bytes belonged to completed windows evicted without a hit. These are cache counters, not physical disk traffic. Cross-run timing means are not controlled performance comparisons.

## Audited native pipeline

The renderer calls runtime Loader::requestFilePartialLoad (0xFA2B0). Its internal call at 0xFA30F invokes 0xFF080, which assigns a native request ID using loader+0x550 and appends a 0x188-byte FileToLoad to a per-thread batch or the pending list. Construction at 0xFA320 fills listener +0x138, user data +0x140, caller destination +0x148, size +0x150, offset +0x158, completion thread +0x160, priority +0x16C. ID +0x178 is populated before construction. The two construction calls are 0xFF15C and 0xFF280.

Selection uses 0xFC5E0 or 0xFBDC0. Copy helper 0xFC4E0 preserves request ID +0x178. processFileLoads (0xFC9B0) dispatches other-thread completion work via 0xFCAE3 -> 0xFB440, or loader-thread work via 0xFCAEA -> 0xFBAE0 for thread value 6.

0xFB440 resolves/opens the virtual file, determines size or seeks a partial range, uses the caller destination when suitable, or acquires/allocates buffers. It contains buffer-availability waits and an interleave-client wait. The actual synchronous virtual read is at 0xFB90F, followed by close at 0xFB922. A LoadedFile record is then published to the requested completion thread's list under its lock. processCompletedFileLoads (0xFCFB0) removes a record under that thread's lock, invokes listener callbacks at 0xFD186/18C/1AA/1AF according to flags, and conditionally releases loader buffers at 0xFD1BA.

Cancellation by ID (0xFB1A0) searches request IDs at +0x178 in pending lists under native locks. Full in-flight cancellation, callback lifetime, decode format and retirement behavior are not yet established. This audit does not authorize replacing those contracts. File/archive resolution must also be made available safely before worker execution for actual early I/O submission.

## Diagnostic implementation

Five startup-only validated direct-call patches observe the partial-load internal request call, its two record-construction calls, and the two worker dispatches. The request wrapper establishes a thread-local loader identity; records store only numeric owner identity, request ID, timestamp and size. Execution matches exact owner/ID; no filenames, game buffers, listener objects or callback pointers are retained or later dereferenced.

The table has 4,096 slots and a try-only atomic lock. Contention drops telemetry instead of waiting under native locks. Collisions replace entries but never produce a match to the wrong owner/ID. Cancelled or unobserved requests can leave stale entries until replacement; replaced is NOT a cancellation count. Unmatched executions include untraced request types, entries older than installation and dropped/replaced records. Only regular partial loads through this verified call are tracked; this is not all game loading.

queue_ms measures construction to worker dispatch, including possible batch delay. worker_ms measures the matched other-thread worker's entire execution (open, buffer handling/waits, read, publication), not disk-only or decode time. loader_ms measures the matched thread-6 path including its native listener work. Maxima and totals are reported every five seconds. These durations are not additive frame costs. Native functions, scheduling, buffers and callbacks remain in control.

## Run

Install the complete package beside Control_DX12.exe and run Start-CPU-R235-LoaderTrace.cmd. The launcher sets CONTROLFG_DIRECTSTORAGE=0 and CONTROLFG_LOADER_TRACE=1, preserving 8 ms / 16 pinned partitions and disabling preparation phase hooks. This avoids R234 duplicate prefetch work during measurement. Only one pinned run through the problem area is needed; send compact logs and texture/stutter observations.

Expect LOADER_R235 installed=1 and LOADER_R235_QUEUE with increasing queued/matched counts. DIRECTSTORAGE_R230_INSTALL ready=0 is intentional for this launcher. This is a diagnostic, not a completed asynchronous DirectStorage replacement or a speedup claim.

Portable tracker tests cover exact owner/ID matching, consume-once behavior, collision rejection, bounded replacement, reversed timestamps, and nonblocking contention. The supplied runtime is independently audited by tools/audit-native-loader.py. Windows CI compiles the production integration and runs the existing direct-call patch fixtures plus the tracker tests. Live matching and observed lead time still need validation.
