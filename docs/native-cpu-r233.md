# R233: DirectStorage cache on the active archive reader

R232 live evidence: 18,254 synchronous reads, 3,986,289,351 bytes, 283/286 sampled handles identified as .rmdp; 250.657 ms across 286 timed samples, maximum 7.470 ms. The asynchronous callsite remained unused. Samples include loading and traversal; these are not frame-time savings.

R233 connects the existing 32 MiB DirectStorage raw archive cache to synchronous reads, restricted to registered-worker PackFile calls. It does not replace native decompression or texture upload, nor implement GPU decompression of game assets.

## Scope and position correctness

The exact audited PackFile::readFile tail dispatch at runtime RVA 0x19671B is redirected through a scoped wrapper. Before that dispatch, native code selects a handle from pack+0x2A0 using native thread index 0x1676E0, opens that slot independently when empty, and seeks to the archive entry offset. The thread-index helper uses TLS and registered OS thread IDs. Unregistered threads return zero, which can share a slot; those reads are expressly excluded.

The wrapper checks that the dispatched function is the audited WinFileSystem::readFile (0x193390). Only a positive native thread index with that implementation enables a thread-local handle scope. Nested scope state is restored on return or C++ unwinding. Other file systems and callers retain native behavior. Exact module SHA and instruction checks precede installation.

Within that scope the existing ReadFile hook queries the current OS position. A completed cache hit copies exact raw bytes, advances the OS position by the requested count, sets the byte count, and returns success. Native WinFileSystem then advances its own logical position as usual. Cache misses, cross-block reads, requests over 2 MiB, EOF/partial reads, invalid positions and unowned handles fall back to original ReadFile. No game buffer or file-handle pointer survives the call. Misses do not wait for DirectStorage completion. Cache setup and first-file discovery still incur overhead, as do metadata and position queries; speedup is not assumed.

The 16 x 2 MiB cache, 16 archive identities, mutation guards, low-priority read-ahead, completion checks, and cancellation remain unchanged. Prefetch reads the current and next block; poor locality can produce wasted I/O. Large and crossing requests deliberately remain native for this first integration.

## Test

Install the full package beside Control_DX12.exe, including dxgi.dll, dstorage.dll and dstoragecore.dll. Run Start-CPU-R233-DirectStorage.cmd. One pinned run through the problem area is enough; send compact logs and texture arrival/stutter observations. Settings remain 8 ms / 16 partitions, preparation phase hooks off, accepted rendering unchanged.

Expect DIRECTSTORAGE_R233 pack_scope=1. DIRECTSTORAGE_R233_SYNC_CACHE reports eligible, served, served_bytes, native_fallback and position_failures. Increasing served proves native reads consumed DirectStorage bytes. R230_STATUS calls/served still describe only the asynchronous hook; its cache byte counters cover both consumers. R233_SYNC sampled time now covers the synchronous cache-or-native path, including cache lookup overhead, but excludes sampled path identification. Compare workloads carefully.

Windows fixtures cover the executable tail patch and rollback, registered-worker eligibility, shared slot zero/unscoped fallback, exact cache-hit bytes, OS and native logical positions, LastError preservation, partial EOF and existing cache ownership/cancellation tests. Live gameplay validation remains required.
