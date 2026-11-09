# R238: native queue order and completion timing

R237's latest pinned run served 388,856,123 bytes through early reads, about 10.46% of observed read bytes. The user saw no performance improvement and still saw slow texture arrival. This build changes admission order and adds downstream timing; it does not establish a frame-rate improvement or replace native texture decoding/upload.

## Run

Copy the package contents beside Control_DX12.exe, including the root dxgi.dll, dstorage.dll and dstoragecore.dll. Exit the game first. Run **Start-CPU-R238-QueueStorage.cmd** for one normal traversal through the problem area. Send compact logs and observations about texture arrival and stuttering. No Reference run or standalone validator is required.

The launcher retains 8 ms / 16 pinned partitions, Native Blend visual behavior, and disabled phase hooks/material resolver. An 8 ms streaming budget permits more native work than 4 ms; it is not a promise of twice the loading speed or higher FPS.

## Changes

R237 offered the first 16 eligible records seen during lookup, before native sorting. R238 snapshots the next records in the native selector's actual tail order at its final record-copy call. It excludes the selected record, examines at most 32 records and copies at most 16 eligible ranges of at most 2 MiB. The selection lock protects the snapshot; only copied IDs/ranges cross the hook. The offer occurs after native selection returns and releases its locks. Subsequent selections refresh the snapshot, so a cold archive can become eligible later.

The native queue, request priorities, callback limits and allocation policy remain native. The backend retains its 16 buffers / 32 MiB cap, 250 ms lifetime, try-lock handoff, identity/offset/epoch checks and native fallback. Pending destinations cannot be recycled before DirectStorage completion. This build does not evict existing early reads to make room for a new snapshot.

Completion observers time six runtime call sites and four renderer import calls to processCompletedFileLoads, plus its releaseBuffers call. Nested completion calls are excluded from completion totals to prevent double counting. Buffer-release timing is an included substage, not additional independent time. At native buffer waits, two aligned atomic snapshots record outstanding completed loads and heap-buffer counts. These are concurrent snapshots, not proof of causality or per-frame latency.

## Expected logs

- `LOADER_R238_QUEUE`: snapshots, eligible candidates and rejected list shapes. `shape_rejected` should remain zero.
- `LOADER_R238_COMPLETION installed=1`: both exact module hashes and all eleven completion/release call sites validated.
- `LOADER_R238_STAGE`: `complete_runtime`, `complete_renderer`, `release_buffers` counts, cumulative time and maximum call time.
- `LOADER_R238_WAIT`: wait sample count, sums/maxima of completion and heap counts, nested completion count.
- Existing `DIRECTSTORAGE_R237 ready=1` and `DIRECTSTORAGE_R237_STATUS` remain the backend's status labels; compare served bytes, capacity/late misses and unused bytes.
- Existing R235/R236 metrics remain available. Pack-read time includes ReadFile time; neither should be added together. Aggregate overlapping worker times are not frame-time savings.

Completion scope includes native callbacks and buffer release. It does not separately identify decompression, texture creation, GPU upload or time until a texture becomes visible. Missing/unsupported modules or call bytes disable the affected observer rather than patch an unknown version.

## Audited native contracts

Runtime SHA256: `a79234e4b34c2488deaac3459e8054be206b5af78a37723d834c1cbd826ce484`.
Renderer SHA256: `ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433`.

Queue copy: runtime RVA FC8F6 -> FC4E0. Selection list pointer +508, count +510, stride 188; selected record is the last element before decrement. Fields use the R236 verified partial-range contract.

Completion calls: runtime FCF82, 130E63, 131ABE, 1346FD, 13509E, 15C1CB -> FCFB0. Renderer 10C90B, 10CA06, 10D38A, 168771 -> import slot 5E07C0. Release FD1BA -> F9FE0. Wait snapshots: loader +818 and +54C. Each call was checked from its PE unwind function boundary, not by a byte-pattern scan alone. Hooks validate original targets at install, use near relays, and roll back partial installation.

## Validation

Windows CI exercises native argument/return/LastError forwarding, sorted-tail admission, selected exclusion, bounded scanning, invalid queue-shape rejection, nested completion accounting and wait snapshots. Existing real DirectStorage tests cover exact bytes/position, no demand submission, ownership, identity, epoch invalidation, expiry and pending-buffer retirement. Existing shader, reflection and native-menu checks remain in the build. Game traversal and any performance benefit still require the user's run.
