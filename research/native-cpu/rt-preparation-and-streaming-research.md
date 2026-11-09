# RT preparation and texture streaming research — 2026-10-30

## Agreed goal — updated 2026-10-30

Pursue DirectStorage as the longer-term texture-streaming direction. The user's accepted responsiveness target is the existing 8 ms setting: textures appear nearly immediately. Treat 4 ms as the user's minimum acceptable setting, not the target. Compare any replacement against 8 ms texture responsiveness, traversal behavior, and CPU/frame-time cost. These are user-observed quality requirements, not measured mandatory CPU durations.

The existing 8 ms option also selects 16 scan partitions. Preserve that exact combination as the reference until independent partition testing establishes a better policy. Native 1 ms/8 partitions remains a diagnostic control, not the target quality baseline. No shipped default has been changed by this goal update.

Identify the native archive codec, chunk/mip layout, loader completion ownership, resource publication and retirement before choosing a DirectStorage insertion point. A locally generated converted cache is a candidate if required for compatible GPU decompression, not an approved implementation design. Success requires preserving the 8 ms experience while reducing CPU/frame-time cost, with no missing or delayed textures.

RT preparation remains a separate performance workstream: measure its internal stages, then evaluate persistent static scene data and incremental updates. GPU instance/shader-table assembly is conditional on ownership, invalidation and dependency evidence. R226 Resolver remains rejected and disabled in the working startup path.

## Evidence from this game

Native run probe-20261030-125056-676-64576, presents 7260–10140: sampled averages material update 1.432 ms, RT preparation 3.235 ms, RT recording 1.013 ms, preparation join 2.366 ms. Join overlaps work and may execute other jobs. These values are not additive and do not establish streaming cost.

The same log confirms requested_ms=0 applied_budget_ms=1.000 applied_slices=8. Source src/probe.cpp ApplyTextureStreamingExperimental sets Native to 1/8 and every nonzero mode to requested milliseconds/16. Thus 4 ms also changes the scan partition count. The menu description that Native leaves the value unchanged is imprecise: the implementation actively writes known native values.

Read-only audit tools/audit-native-streaming.py reproduces the selected instruction checks and writes streaming-audit.json, including renderer SHA256. RVAs below apply only to that binary.

- 0x15AB30 reads the budget at 0x9186A0, divides by 1000, compares it against an accumulated duration at manager+0x80, and conditionally calls 0x158060. A bypass condition also exists. Both result paths add elapsed timer duration to the accumulator. This is admission control around individual request work, not a hard cap on total pipeline time. A single admitted operation can exceed the remaining budget. Reset cadence still needs tracing.
- 0x158247 calls imported r::Loader::requestFilePartialLoad from the request body. This proves an existing partial-load interface, not the disk codec, full asynchronous execution contract, or where time is spent.
- 0x168ED0 reads the resource pool under its native lock. At 0x169138 it loads the setting at 0x919010 into r14. It resets the phase if phase>=slices, walks entries at indices phase, phase+slices, etc. (0x1691C5), then increments phase (0x169255). Hence 16 partitions means approximately half the entries visited per invocation and twice the invocations for a full cycle compared with 8, assuming an unchanged pool. Do not translate invocations to frames until scheduling is established.
- This routine also consults native video-memory information and performs pool-control work. Our current budget patch changes neither residency policy nor disk/decompression machinery.
- RT preparation 0xA81B0 contains object query (indirect call 0xA83D9), readback map/process/unmap (0xA840B/0xA8431/0xA8448), object/geometry traversal with helper 0xA8CE0, TLAS prebuild sizing (0xA8810), buffer allocation helpers (0xA889D/0xA88E1/0xA8944), and lock/copy/unlock upload (0xA8972/0xA8987/0xA8998). Calls named alloc may reuse capacity; map does not prove a GPU stall. Measure before replacing either.

## Primary-source research

1. NVIDIA, Practical Tips for Optimizing Ray Tracing:
https://developer.nvidia.com/blog/practical-tips-for-optimizing-ray-tracing/
Supports separating static/dynamic processing, caching static data on the GPU, batching work, and incrementally updating persistent geometry/material data. Reflections and shadows need off-camera objects: aggressive camera-frustum removal would violate our visual requirements. These are architectural candidates, not proof Control can adopt them safely.

2. Microsoft, DirectStorage developer guidance (Windows):
https://github.com/microsoft/DirectStorage/blob/main/Docs/DeveloperGuidance.md
Supports batched reads, CPU custom-decompression integration, GPU destinations with explicit layouts, and completion synchronization. Texture destinations expect copy-compatible layout. Full queues can block enqueue; per-request synchronization should be avoided where possible. CPU-memory destinations use CPU decompression. Replacing a file-read call alone does not implement GPU decompression.

3. Microsoft, DirectStorage 1.3:
https://devblogs.microsoft.com/directx/directstorage-1-3-is-now-available/
Adds batched EnqueueRequests with fence coordination and contiguous mip/subresource-range destinations. These match staged mip streaming in principle; engine ownership and resource transitions still have to be integrated.

4. Microsoft SDK download listing and 1.4 announcement, checked 2026-10-30:
https://devblogs.microsoft.com/directx/directstorage-api-downloads/
https://devblogs.microsoft.com/directx/directstorage-1-4-release-adds-support-for-zstandard/
Listing identifies 1.3.0 and 1.4.0-preview1-2603.504; 1.4 article describes public-preview Zstd CPU/GPU decompression. Treat preview support and driver-specific performance as experimental. Neither article establishes the codec in Control's actual assets.

5. NVIDIA, GDeflate/DirectStorage:
https://developer.nvidia.com/blog/accelerating-load-times-for-directx-games-and-apps-with-gdeflate-for-directstorage/
Explains GPU-friendly compression and the importance of sufficiently batched, appropriately packaged data. GDeflate is not an automatic decoder for an arbitrary existing archive codec.

## Options and prerequisites

| Candidate | What it could improve | Required evidence |
| --- | --- | --- |
| Independent 4 ms budget with native 8 partitions | Faster admission without slower scan coverage | Request time, scan cadence, completion latency, frame tails |
| Native loader batching/prefetch/priorities | Fewer small requests and earlier needed mip arrival | Request ordering, cancellation, ownership, archive/chunk boundaries |
| Budget-aware resident-mip retention | Avoid repeated eviction/reloads | Actual pool occupancy and eviction/reload rates; Windows VRAM budget |
| DirectStorage reads with existing CPU decode | CPU overhead of storage submission | Loader I/O is significant; correct callback delivery and fallback |
| DirectStorage GPU decode/upload | CPU decompression and staging overhead | Compatible codec/chunks/layouts, destination ownership, barriers/fences |
| Local converted asset cache | Provide compatible chunks if originals cannot be read directly | Measurable gain justifies conversion time/storage; versioned invalidation and native fallback |
| Persistent RT instance data | Repeated extraction and assembly | Stable identities plus invalidation on transforms, destruction, streaming, LOD, material remaps |
| GPU instance/table generation | CPU packing/upload work | Persistent GPU inputs and no immediate readback dependency |

Large RAM/VRAM capacity alone does not establish a safe pool size. Do not force all textures resident or freeze material descriptors. Preserve low-resolution fallback until new mips are complete; never publish an incomplete texture. Keep native ownership, completion callbacks and retirement behavior until a replacement contract is proven.

## Next implementation scope

1. Split sampled RT preparation timing into query/readback, object/geometry processing, sizing/allocation, and upload. Use per-job intervals and bounded aggregation, not a timer/log per object. Include instrumentation-off comparison.
2. Independently expose budget and partitions in diagnostic configuration. Test 1/8 baseline, 4/8 (budget only), and 4/16 (existing combined behavior); also measure 8/16 as the accepted responsiveness reference and 8/8 to isolate partition effects; 8 ms is the requested target. Do not change the shipped menu default yet.
3. Trace streaming request admission, requested bytes/mips, in-flight counts, completion publication, and residency churn. Measure CPU work separately from end-to-end latency. Trace accumulator resets and scan invocation cadence before labeling per-frame results.
4. Identify archive codec and chunk offsets from actual loader/runtime and representative texture assets. Unknown at this checkpoint. Do not claim a DirectStorage implementation before that contract is mapped.
5. If I/O/decompression is substantial, prototype one texture class with native fallback. Compare decoded bytes, mip completeness, cancellation/teardown, GPU resource retirement, camera turns/traversal and repeated loads. A standalone DirectStorage bandwidth benchmark is insufficient.

Acceptance: repeatable median and tail frame-time improvement, equal scene workload, no delayed/missing textures or worsening time-to-detail, no reflection/light regressions, no unbounded memory growth. Compare stationary views and traversal separately, and distinguish cold from warm cache. Keep R217 visuals, Map Guard OFF, angled reflections and native streaming callbacks intact.

No new performance build or runtime streaming modification was produced by this research. Static audit passes; Windows/game measurements are the next phase.
