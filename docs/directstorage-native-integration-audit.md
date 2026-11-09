# Native DirectStorage integration audit — 2026-10-30

## Objective and acceptance criteria

The user's objective is to reduce CPU-side stalls that leave the GPU underfed in CPU-bound areas, while improving texture arrival and traversal consistency. Average FPS is secondary. The user further requires the next payload implementation to target GPU resources directly, with GPU decompression where needed; another CPU-buffer delivery stage is not the requested milestone. CPU metadata, resource creation and submission remain necessary. Keep 8 ms / 16 pinned streaming partitions and the accepted Native Blend visuals.

Measure graphics-queue starvation attributable to loading, CPU critical-path waits, request-to-mip-ready latency and traversal frame-time spikes. Aggregate GPU utilization alone is insufficient: adding decompression work could increase that percentage without feeding rendering sooner. Separate startup/loading-screen intervals from traversal and steady scenes. RT scene preparation remains an independent CPU bottleneck; DirectStorage cannot be credited with solving it without evidence.

## Research basis

Microsoft's [PC developer guidance](https://github.com/microsoft/DirectStorage/blob/main/Docs/DeveloperGuidance.md) recommends batching, sufficient queue/staging capacity and avoiding serial read dependencies. System-memory destinations use CPU decompression. [EnqueueSetEvent](https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue1-enqueuesetevent) supports notification after preceding requests complete. The [BulkLoadDemo](https://github.com/microsoft/DirectStorage/blob/main/Samples/BulkLoadDemo/README.md) separates CPU metadata from GPU payloads and prepares texture layouts for GPU destinations. [Microsoft's GPU-decompression explanation](https://devblogs.microsoft.com/directx/directstorage-1-1-now-available/) requires appropriately compressed streams and eligible destinations. Xbox hardware-decompression guidance is not a specification for this Windows implementation.

The packaged SDK remains pinned at 1.3. This audit does not require an SDK upgrade; the examined event APIs already exist in its local header. No assumption that arbitrary Control archive bytes are GDeflate is valid.

## Current implementation: real API use, incomplete engine integration

Reviewed `src/directstorage_early_reader.h`, `src/native_loader_early_runtime.h`, `src/native_loader_stages_runtime.h` and `src/native_loader_completion_runtime.h` at local commit d4aabf0 / remote d9e6491b480b4f6ec3a120e414c998164ed82fad.

The current path is native selection -> speculative DirectStorage reads into private CPU buffers -> intercepted synchronous ReadFile consumes a ready result -> native callback and texture-upload processing. It is an in-process read-ahead bridge, not an authoritative asynchronous asset loader.

| Finding in our code | Consequence for the objective | Required correction |
|---|---|---|
| MEMORY destination, compression NONE | No GPU decompression or direct texture destination | Treat this as raw archive transport only; GPU work requires a separately verified payload/upload contract |
| Native read still runs when an early request is pending | Duplicate I/O can remain in flight; native worker can still block | Give accepted requests one I/O owner; resume from completion instead of racing a second read |
| Metadata query on demand; position checks and memcpy on hits | Old filesystem semantics and CPU copy overhead remain | Own archive mapping at registration and integrate above synchronous ReadFile |
| 16 request slots, each reserving 2 MiB | Small reads consume a whole slot; request capacity can bind independently of memory bytes | Size request descriptors independently from a byte-budgeted buffer allocator |
| Worker wakes on admission and polls active jobs at 2 ms intervals | Frequent submissions and polling remain; completion recognition is not event-driven | Use a bounded batch submission worker and completion events, with explicit batch ownership |
| Per-request status markers on one queue | Readiness waits for preceding work, not just an individual physical read | Choose notification groups by asset/priority/deadline rather than assuming independent per-read readiness |
| A new snapshot is offered after every selected request | Many repeated admission attempts; no authoritative pending queue | Track accepted request state once and apply bounded backpressure |
| Early reader requires LOADER_TRACE=1 | Optimization depends on diagnostic instrumentation | Separate integration hooks from optional sampled diagnostics |
| Completion observer times every polling call | Observer itself adds work to extremely hot native loops | Remove per-poll timing before performance judgment; observe useful batches or bounded samples |

Persistent queues, exact range validation, archive identity checks, owned I/O buffers and delayed retirement are useful foundations to retain. Passing these correctness tests does not establish reduced CPU starvation.

## R238 evidence

Source: attached `Control-FG-Compact-Logs-20261030-155528-94994a01.zip`, latest session only: `probe-20261030-205342-865-81156.log`. Final report present 6840.

- 4,633 submitted requests / 3,620 submissions: **1.28 requests per submission**. Startup at present 60: 688 / 643, approximately 1.07.
- 384,039,215 served bytes out of 3,683,888,772 observed bytes: **10.42%**. These counters cover observed archive reads, not texture-only traffic.
- 1,472 late requests and 116,124,656 unused submitted bytes. A late status does not independently identify physical storage latency.
- 75,943 capacity rejections are repeated offer attempts, not 75,943 distinct missed assets.
- Zero I/O, identity, position and resolved-range mismatch errors; queue-shape rejection zero. 8 ms / 16 partitions stayed pinned.
- Renderer completion observer: 254,807,641 calls, of which 254,513,563 were already counted by present 60. This is primarily startup evidence, not proof of traversal's CPU bottleneck.
- Buffer waits totaled 2,692.802 ms across 920 calls; maximum 1,175.985 ms. These are wall-clock worker intervals, not additive CPU execution or frame costs. They lack correlation with graphics starvation.

## Newly traced texture-specific boundary

Renderer SHA256 `ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433`; runtime SHA256 `a79234e4b34c2488deaac3459e8054be206b5af78a37723d834c1cbd826ce484`. Addresses are RVAs. Export names and decoded instructions establish the following control flow, not full recovered source.

| Stage | Verified boundary | Significance |
|---|---|---|
| Mip request | Renderer TextureResource::requestLoadLevel 158060; imported requestFilePartialLoad call 158247 | Concrete texture-only admission site. It supplies byte offset/size, the texture listener, completion-thread ID and level as user data |
| Completion callback | TextureResource::notifyFilePartialLoadComplete 158270 -> loadLevel 158280 | Maps completion user data back to the mip-level index |
| Native upload preparation | loadLevel calls 1546E0 at 15831A | Dispatches to texture-type-specific processing |
| 2D path | 1546E0 -> 154C80 | Calls native texture lock/unlock, reads from the supplied stream and then follows native copy/publication work |
| Cube/volume paths | 154A60 / 1547A0 | Include imported NativeTextureUtil::updateSubresource calls; require separate layout validation |
| Publication handoff | loadLevel call 158374 and finishLoadLevel call 1583FE -> 160370 | Follow-up record contains texture pointer, level and success flag. Consumer, synchronization and GPU completion semantics remain to be mapped |

The 2D path has been traced to native lock/unlock and copy calls, not through their D3D12 implementation. This is not yet proof of the final GPU-ready instant or of a particular compression codec. PE unwind fragments can split a function: loadLevel continues beyond the fragment ending 158296.

The renderer loop around 10C8DE tests MultiProducerFifo::getPacketsWaitingCount and another object field, then calls processCompletedFileLoads at 10C90B and loops. A high call count is therefore not a count of loaded assets. Do not replace it with sleeps or skip callbacks without mapping what satisfies its exit conditions.

## Implementation direction

1. Separate functional integration from diagnostics. Eliminate per-iteration completion timing, and retain inexpensive batch-level correlation. Keep the existing early-read build as an experimental reference, not the final architecture.
2. Reconstruct the texture-specific ownership boundary above. Verify listener/resource retention, unload/cancel races, requested completion thread, pre/post-load listeners, native buffer ownership, error/retry behavior and final publication. Trace actual format and mip layout through the native D3D12 upload implementation. Do not broaden to a full-game decompile.
3. Implement one authoritative asynchronous GPU-destination backend for a narrow supported texture-mip class. Establish the real D3D12 resource, mip footprints, compression format, residency, state transitions, queue synchronization and lifetime. Accepted payloads go to their GPU resource, not our private CPU buffer and memcpy bridge. A completion notification schedules the required native publication; rendering must not access unfinished payloads. Unsupported assets stay native and are reported explicitly, never counted as GPU-routed assets.
4. For compressed payloads, verify GPU decompression support and the actual chosen path. Do not silently count CPU decompression fallback as meeting this goal. Native uncompressed GPU-ready payloads need transfer, not artificial decompression. CPU metadata, resource creation, request submission and synchronization remain necessary; DirectStorage can internally stage transfers through system memory. GPU destination does not promise a physically direct SSD-to-VRAM transaction.
5. If the native layout/codec is incompatible, investigate a versioned local GPU-ready derived-content cache built from the user's assets outside gameplay. Preserve original assets and invalidate derived data by source/version identity. Do not transcode during traversal, route GPU results back through CPU buffers, or substitute a CPU-buffer intermediate build for the requested GPU payload milestone. Failure recovery and unsupported paths must preserve native behavior without racing writes or premature resource retirement.
6. Judge the result by reduced streaming-linked graphics starvation, CPU critical-path waits and mip-ready delays with stable visuals and traversal frame times. Keep steady RT-preparation stalls separate. No additional user capture is needed to establish this architectural correction; existing logs and binaries suffice for the audit. A later integrated build still needs a game run.

No runtime patch or new game build is introduced by this research commit. The next implementation must establish GPU payload destination, request ownership and native completion delivery, rather than only increasing speculative cache coverage.
