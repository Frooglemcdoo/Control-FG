# R224: identify changing material records

R223 found 3,687 changed-material-hash writes and 132 texture callbacks during 2,520 steady material updates. No other tracked writers fired in that interval. This attributes 96.5% of tracked steady invalidations to material hash changes, but does not identify the affected entries or prove their changes redundant.

## Run

Install over R223. Run `Start-CPU-R224.cmd`, stand in the same problem-area view for 30–60 seconds without video recording, exit, then run `Collect-ControlFG-Compact-Logs.cmd`. Send its ZIP and FPS range. All original updates, R217 visual fixes, Map Guard OFF and angled reflections remain intact. Both append experiments remain bypassed.

## New measurement

The only identified direct call to native update routine 0xC7A20, at 0xC7987, now forwards through a diagnostic wrapper. It snapshots the saved 336-byte material record, eight-byte key and stored hash before the native update. It always forwards all four arguments and preserves the native result/state. After a stored-hash change, one in eight such calls per thread records the manager address, entry index, old/new hash/key, all changed dword offsets as two bitmasks, and up to eight raw before/after examples. It captures native field +0x148 before update and its committed value: the native routine hashes the incoming record before rewriting that field. No field is suppressed or treated as irrelevant.

The wrapper runs within the existing caller's material-update ownership. Native 0xC7A20 accesses those arrays without resizing them; snapshots retain no game pointer for later dereference. A fixed 1,024-event queue uses a single nonblocking try-lock. Contention/full capacity drops diagnostic events and increments a counter. Log formatting/writes happen from Present at the existing reporting cadence, not while holding the native material lock. The wrapper adds a 336-byte copy per eligible call; R224 is not an overhead-free FPS benchmark.

`NATIVE_CPU_R224_INSTALL ready=1` confirms exact callsite/callee bytes and successful installation. `NATIVE_CPU_R224_DIFF` includes `entry`, `old_hash`, `new_hash`, `old_key`, `new_key`, `source_derived`, `committed_derived`, `changed_words`, `mask0`, `mask1`, and `words`. Example `c:00000000>3f800000` means dword offset 0xC changed between those raw bit patterns. Masks cover all 84 dwords even when examples truncate at eight. The `present` value is when the buffered event was drained, not its original frame. IDs are process-local and may be recycled; periodic sampling does not provide unbiased per-entry frequency estimates.

`NATIVE_CPU_R224_STATUS` reports total observed hash changes, sampled events reported, dropped events and diagnostic bound/nullable-storage skips (`invalid`). Invalid diagnostic inputs still forward to native. Texture pointers may change without a committed record-byte change; a zero-byte-diff event is not permission to skip an update. R223 counters remain available for cross-checking coverage.

Set CONTROLFG_CPU_MATERIAL_DIFF=0 to omit this wrapper. The supplied launcher explicitly enables R218/R222/R223/R224 diagnostics and bypasses append experiments. No incremental GPU updates or caching are introduced yet.

## Validation

Portable tests cover all 84 word offsets, both masks, example truncation, derived field capture, full/wrapped queue handling, concurrent publication and exact drop accounting. Windows runtime tests cover four-argument forwarding, LastError, native committed changes, one-in-eight sampling, unchanged hashes, invalid entry bounds and null handles. Existing R223 probe/unwind and visual checks remain in CI.
