# R230: DirectStorage connected to native archive reads

Install the full package, including the **DirectStorage** folder beside dxgi.dll. Exit Control and run **Start-CPU-R230-DirectStorage.cmd**. Use the same problem area and traversal, then collect compact logs and report texture arrival, stutter and FPS. No Reference run or standalone validation is needed.

The launcher retains the working 8 ms budget and 16 pinned partitions. Phase hooks remain off. R229 successfully held these values in all 26 reported snapshots; the user still wanted faster texture arrival and requested DirectStorage integration before partition changes.

## What is integrated

The exact verified WinFileSystem::readFileAsync ReadFile call now checks a DirectStorage-backed cache of real .rmdp archive bytes. On a miss it schedules the current and following 2 MiB archive blocks for DirectStorage read-ahead, then forwards the request to native ReadFile. On a completed cache hit it copies the exact requested bytes and completes the native FileEvent immediately. It does not wait for a pending cache block.

This is an initial in-game read-ahead backend, not a replacement for the entire asynchronous loader. It uses DirectStorage file-to-CPU reads without decompression. Native archive interpretation, texture decoding, GPU upload and publication stay in place. The previous GDeflate validator remains separate; its GPU-decompression result does not mean native textures now use GPU decompression.

## Correctness and limits

- 32 MiB of owned data buffers, 16 pending/ready blocks, at most 16 open archive identities. Queue capacity exceeds the maximum outstanding requests plus status markers.
- Never retain the game's output buffer, OVERLAPPED, event or native file-handle pointer. DirectStorage writes only owned cache buffers.
- Consume blocks only after successful completion. Failed DirectStorage requests disable cache consumption and new prefetch; native reads continue.
- Native fallback for pending/missing data, lock contention, cross-block or oversized reads, EOF, unsupported paths and initialization failure. No waiting for cache I/O on the game thread, though initial file opening and identity queries still have overhead.
- Match native, guard and DirectStorage file identities/size/write time. A process-owned read guard prevents archive mutation/deletion while cached. Files remain open until game exit; close Control before modifying game archives.
- Reuse only completed slots. Cancellation prevents further cache consumption; explicit test teardown waits for safe completion. Production resources remain owned for the process lifetime, with no teardown work in DllMain. Hot DLL unloading is unsupported.
- The hook is limited to the hash-verified native asynchronous callsite. It does not intercept arbitrary ReadFile users, IOCP/APC contracts or synchronous file-position reads.

Prefetch can waste I/O or increase CPU/memory-copy work if accesses do not reuse nearby blocks. No performance gain is assumed. The native fallback guarantees correctness on misses, not equal performance. This build measures whether this insertion point and locality help before committing to a broader backend.

## Proof in compact logs

```
DIRECTSTORAGE_R230_INSTALL ready=1
DIRECTSTORAGE_R230_STATUS ... calls=... served=... native_fallback=... served_bytes=... prefetched_bytes=... io_failures=...
STREAMING_R229_STATE ... pin_installed=1 budget_ms=8.000 slices=16
```

`ready=1` proves installation only. Increasing `served` and `served_bytes` prove game reads actually consumed completed DirectStorage data. Zero hits means no demonstrated benefit, even if prefetch is active. `prefetched_bytes` includes read-ahead that may never be used; do not interpret it as saved native I/O. Counters are process-cumulative.

Windows tests use real DirectStorage reads and the production ReadFile hook to check byte equality, cold native fallback, completion event/result behavior, nonzero offsets, short tail blocks, bounds/EOF, file identity, mutation exclusion, cancellation and safe drain. Native module/callsite structure is separately audited against the supplied runtime DLL.
