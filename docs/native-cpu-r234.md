# R234: request-sized DirectStorage read-ahead

R233 successfully served 9,362 native reads (385,875,752 bytes), but submitted 19,228,848,416 bytes of prefetch requests. That is approximately 49.8 submitted bytes per served byte, not a measurement of physical disk traffic. R233 had no logged read, position or DirectStorage I/O failures. Its workload differs from R232, so sampled means do not establish a performance gain.

R234 removes the unconditional next-2-MiB fetch. A miss now submits at most one window covering the requested range, rounded to 64 KiB boundaries and clipped at EOF. If rounding an unaligned near-2-MiB request would exceed the allocation, it submits exactly the requested range instead. Hits submit no I/O. Existing pending or ready ranges that cover the demand suppress duplicate submissions. Reads crossing the former 2-MiB grid can now hit after their covering window completes. Requests larger than 2 MiB remain native.

For an isolated 1,367-byte demand at offset 713, the submitted request is 64 KiB instead of 4 MiB: 64 times smaller. This is a deterministic policy result, not a promised gameplay improvement. Narrower windows may also reduce future hits; the live capture must show whether the traffic reduction is worthwhile.

The allocation cap remains 32 MiB across 16 slots; actual populated bytes per slot can now be much smaller. This build does not increase slot count or cache capacity. Registered-worker ownership, native position updates, immutable archive guards, native miss fallback, decode and upload behavior remain as in R233. No cache-completion wait is added.

## Run

Extract the entire package beside Control_DX12.exe, including dxgi.dll, dstorage.dll and dstoragecore.dll. Run Start-CPU-R234-DirectStorage.cmd. Traverse the problem area and send compact logs plus texture-arrival and stutter observations. One pinned run only, 8 ms / 16 partitions, preparation phase hooks off.

Expect DIRECTSTORAGE_R234 pack_scope=1 window_kib=64 next_block_prefetch=0. Existing R233_SYNC_CACHE served and served_bytes identify actual consumed cache data. R234_CACHE adds submitted_requests, submitted_bytes, unused_evicted_bytes, pending_covered and capacity_misses. Unused evictions count completed windows evicted without a cache hit, not all unused bytes and not physical disk traffic. R230_STATUS asynchronous calls/served can remain zero; its shared prefetch byte totals include the synchronous cache.

## Validation

A portable range test covers a million randomized ranges plus zero, oversized, EOF, overflow and unaligned-maximum requests. Windows DirectStorage fixtures verify real byte equality across former block boundaries and for maximum-size unaligned requests, a single 64-KiB small-demand submission, no next-block submission, no duplicate pending/hit submissions, and existing native position, fallback, identity and cancellation behavior. Live gameplay validation is still needed.
