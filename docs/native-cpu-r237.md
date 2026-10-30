# R237: bounded early DirectStorage archive reads

R236 verified 4,830 predicted archive ranges against native reads with zero mismatches. ReadFile occupied 99.89% of measured PackFile read duration. The last two reporting intervals had 3.31 ms mean resolved lead and 0.546 ms mean native ReadFile duration. These are workload-specific wall durations, not guaranteed deadlines, physical disk latency, or predicted frame savings.

## What is implemented

R237 submits real uncompressed archive byte ranges from queued native requests to DirectStorage FILE -> CPU_MEMORY on an owned background worker. The native decoder, destination allocation, completion callbacks, upload and request ordering remain in control. This is an initial bounded integration, not GPU decompression or a full streaming-system replacement.

The existing location lookup observer copies at most 16 eligible scalar request descriptors per batch (within R236's 256-record inspection cap). It retains loader/request numeric identity, numeric pack identity and exact byte range. After the native selector returns and has released its locks, a try-only handoff admits requests into a fixed 16-slot / 32 MiB pool. The already-selected request is excluded to avoid starting an I/O request at its own demand point. Eligible reads are at most 2 MiB, use exact ranges, and expire 250 ms after admission. No automatic adjacent-window prefetch occurs.

Archive registration is learned from a real native synchronous PackFile read on a registered thread. The game thread obtains file identity and duplicates the handle for metadata ownership; a background worker obtains the path, checks the .rmdp suffix, opens an independent read-only guard and DirectStorage file, and verifies all identities. The duplicate is never used for reading or seeking, then is closed. File opening and DS submission do not run under native selection locks. A cold archive's first reads remain native; only subsequent eligible queue batches can benefit. There are 16 archive-registration slots; exhaustion falls back.

When the native reader reaches a matching request, R237 can copy only an already-complete successful owned result. It checks exact loader/request identity, pack identity, raw range, OS file identity and current native file position. On success it advances the Windows file position, copies bytes, sets the returned count and preserves LastError; native wrappers perform their usual logical-position updates. Shared native thread slot zero is excluded. Unknown, oversized, full-file-unobserved, wrong-identity, wrong-position, incomplete or unavailable results use the original native read immediately. Demand misses never enqueue new DS reads.

Queued work already overtaken by native demand is discarded when the try-only handoff is available. Submitted work that loses the race to native demand is marked for discard and retains its buffer until DS completion. Contention can prevent immediate discard, so some redundant I/O remains possible and is bounded/reported. This build does not claim to eliminate all wasted reads.

## Ownership audit and invalidation

Native unmount paths 18BDC0 and 18BEE0 call PackFile destructor 1951C0 at 18BE62 and 18BF0E. reloadPackFiles 18BF60 also destroys changed packs at 18C2C2. These three validated direct calls now increment an atomic backend generation before forwarding the original destructor with unchanged arguments and LastError. The invalidation is conservative across all registered archives. Copied numeric pack addresses are never dereferenced by deferred work.

The worker discards invalidated queued/ready results, cancels reuse of old registrations, and releases old owned file handles only after every corresponding submitted destination has retired. A new epoch learns native identities again. Independent identity checks remain required at consumption even if a destruction route is unobserved. Read-only guards prohibit in-place mutation while held; the guard allows delete-sharing, though the DS runtime may have additional sharing constraints. Hot replacement of archives is not a promised feature of this first integration.

Native queued cancellation remains native: a cancelled ID never reaches the consumption hook. Its independent early read may finish unused; it expires/discards without writing to game memory. No native listener, destination, filename pointer, FileToLoad or PackFile object is retained. No synthetic native completion is generated. Process lifetime owns the production worker; hot DLL unloading is unsupported, as with existing runtime hooks. Test shutdown waits for completion retirement before freeing storage.

## Run

Extract the whole package beside Control_DX12.exe. Run Start-CPU-R237-EarlyStorage.cmd, load the level and traverse the problem area. Send compact logs with texture arrival/stutter observations. One pinned run is sufficient; no Reference or standalone validator run is required. The launcher keeps 8 ms / 16 partitions pinned, phase hooks off, the old demand-time cache OFF, and CONTROLFG_EARLY_STORAGE=1.

Expected startup: DIRECTSTORAGE_R237 ready=1, DIRECTSTORAGE_R237_INSTALL ready=1, LOADER_R236 installed=1 candidate_io=1, and LOADER_R235_INSTALL ready=1. DIRECTSTORAGE_R230_INSTALL ready=0 refers to the disabled old cache and is intentional.

DIRECTSTORAGE_R237_STATUS reports archive registration, offered/admitted/cold/capacity counts, submitted bytes/batches, completed requests, served bytes, late/avoided/expired work, unused bytes, generation invalidations and identity/position/I/O failures. Submitted/unused bytes are logical request bytes, not measured physical disk traffic. R236 readfile timings now count only actual native ReadFile calls; served requests bypass that API and still appear in pack_read timing. Do not directly compare read means without checking served/native counts and workload.

## Validation and limitations

Portable range/tracker tests and the supplied-runtime callsite audit remain in place. Windows tests exercise real DirectStorage file reads, exact unaligned bytes and position, no demand submission, owner/file identity rejection, stale-epoch rejection, expiry and shutdown with submitted work. Forwarding fixtures verify the five-argument PackFile ABI, post-selector handoff, nested thread-local scopes, and LastError. Existing rendering/menu and DirectStorage tests remain enabled in CI.

Live benefit is unproven until R237 logs show actual served requests, useful byte coverage and acceptable wasted I/O. Cold-start batches, large reads, capped batch entries and insufficient lead time still use the native path. This build deliberately retains the accepted visual baseline; it does not modify RT preparation or the streaming scan budget.
