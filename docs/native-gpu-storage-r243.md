# R243 GPU streaming startup and burst scheduling

Install over R242 and run `Start-CPU-R243-NativeGPUStorage.cmd`. Use one pinned
traversal and send compact logs with texture-arrival and frame-spike observations.
The launcher retains 8 ms / 16 partitions and the accepted rendering baseline.

## Changes

- Compile the unchanged row-repack shader and serialize its root signature at
  build time. Embed both in dxgi.dll; this streaming path no longer invokes
  D3DCompile or root-signature serialization in the game. Driver pipeline
  creation is still needed once per process/device.
- Share that immutable root signature and pipeline between the two transfer
  lanes after checking native device identity. Each lane retains its own
  queues, command allocator/list, fences and GPU arenas.
- Raise bounded pending descriptors from 2,048 to 4,096. R242 confirmed 150
  actual queue-full fallbacks with zero duplicate or unavailable rejections.
  Pending descriptors retain existing destination texture and engine references;
  more pending descriptors can retain more destination VRAM.
- Select each priority/FIFO batch with one collection pass plus a partial sort,
  replacing a full-array scan for every selected request. The staging budget
  and batch limit remain 128 MiB per lane and 16 requests. Reserve alignment
  headroom in admission and scheduling so an oversized head job cannot stall
  the queue permanently.
- Retain R242 arena reuse and fence-gated native publication. GPU payload read,
  repack and copy remain unchanged; no runtime CPU payload copy is introduced.

`DIRECTSTORAGE_R243_INIT` records queue creation, pipeline setup and command
object/event creation in worker wall microseconds. Lane 0 should report
`shared_pipeline=0`, lane 1 `shared_pipeline=1`; both use `shader=build_embedded`.
These timings include CPU scheduling and API stalls; they are not GPU timestamp
measurements. Partial failed initialization can report zero for an incomplete
stage; always check ready and HRESULT.

R242's 1.710321-second maximum spans the entire initialization call. It does not
identify shader compilation as the cause and does not prove a same-duration
frame stall. R243 removes redundant startup work and measures the remaining
stages; live improvement must still be verified.

## Validation

Windows CI compiles the shader/header before compiling both the GPU tests and
proxy. The existing 72-mip byte comparison uses the embedded shader, including
arena growth/reuse, disjoint concurrent lanes and blocked-consumer completion.
New checks verify shared pipeline use, full-queue priority/FIFO scheduling,
staging-budget splits, empty queues and the 4,096-slot admission boundary.

For manual builds, first compile and run
`tools/directstorage/compile-packed-shader.cpp` from the repository root with
D3D12 and D3DCompiler libraries; create `build` first. It generates
`build/native_texture_repack_bytecode.h`, required by the packed transport.

## Scope and 8 ms budget

The supported asset scope remains the newly allocated, unpublished mode-zero
2D textures described in [R242](native-gpu-storage-r242.md). Unsupported layouts
and overflow still fall back to native loading. Native archives contain raw
BC/RGBA data, not GDeflate streams. CPU resource creation, metadata, scheduling
and native publication remain necessary.

8 ms remains the native streaming budget; it is neither a DirectStorage GPU
budget nor a guaranteed 8 ms frame cost. It is held fixed here to isolate the
startup/scheduling changes. A later pinned 4 ms versus 8 ms comparison is needed
to decide the best default under this integrated path.

## R242 evidence

Latest session in Control-FG-Compact-Logs-20261030-201238-c99d2ecb.zip:
9,154 offered, 9,002 accepted/completed, zero failed batches and zero pending at
the final sample; 2,576,119,960 accepted bytes. There were 18 staging allocations
and 2,544 reused batches out of 2,553. Fallbacks were 150 queue-full and two
unsupported layouts. This demonstrates the pooling change worked, not that
all frame spikes were removed or every in-game texture was byte-validated.
