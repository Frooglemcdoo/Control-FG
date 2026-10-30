# R241 native GPU texture streaming

R241 replaces the native payload read/upload for accepted, newly allocated 2D
texture requests. This is an integrated experimental game path, not the R237
CPU-memory read-ahead bridge or another metadata-only observer. It still needs
live-game validation before it can be considered production-complete.

## Run

Copy the package contents beside Control_DX12.exe. Launch with
`Start-CPU-R241-NativeGPUStorage.cmd`. The launcher keeps 8 ms / 16 partitions
pinned and the accepted rendering baseline. It disables speculative read-ahead,
the legacy cache, loader tracing, completion polling instrumentation and R240
layout sampling. Functional integration does not depend on those diagnostics.

The five-second `DIRECTSTORAGE_R241_STATUS` report distinguishes offered,
accepted, completed, fallback, failed, pending and batch counts. `installed=1`
alone is not proof of successful GPU texture delivery; completed must increase.
Counters include startup and traversal, not just the affected area.

## Payload path

1. The exact renderer allocation call at 1580D7 identifies newly allocated mip
   containers; existing containers remain native. The request call at 158247
   admits supported requests before the native loader reads payload bytes.
2. Native pack lookup 18F760 supplies the archive range. Only filename/range
   metadata is processed on the CPU. Open archive identities are checked and
   held read-only against replacement/write. No archive payload cache is built.
3. A dedicated worker submits up to 16 raw archive ranges per DirectStorage
   batch to DEFAULT-heap GPU buffers. There is no CPU payload buffer, Map,
   memcpy or CPU decompression in this path.
4. A compute shader repacks compact mip rows into D3D12 copy footprints. On the
   engine's direct queue, GPU copies write each texture mip and restore its
   original combined pixel/non-pixel shader-read state. The native state
   tracker is not rewritten.
5. After the GPU fence completes, native finishLoadLevel (158390) dispatches
   publication through the renderer's existing locked deferred-work path.
   Native references (12E160/12E190) and COM references cover accepted jobs.
   Loading mip state 1 blocks normal unload through canFinishUnload (157520).
   Failed I/O retries native only after the accepted DirectStorage batch retires.

The special returned request ticket is local to this exact renderer call site,
whose native caller discards the return value. It is not a replacement for the
public Loader request-ID/cancellation API. Native loader listeners and buffer
allocation are bypassed for accepted requests; renderer completion accounting
is performed by finishLoadLevel.

## Supported scope

Admission requires an unpublished new container, positive engine reference
count, mode-zero 2D metadata, no alias container, no special native texture
flags, DEFAULT heap, one array slice/plane/sample, at most 16 mips, and compact
rows divisible by four bytes. It requires the expected shader-read tracker
state and an exact compact-size match. Requests above 32 MiB, existing
containers, unsupported texture types/layouts, other completion-thread IDs,
capacity pressure and unmatched module hashes use native behavior.

The source bytes in this native path are raw BC/RGBA texture payloads, so
compression is NONE. BC texture formats are already GPU-sampleable blocks;
they are not GDeflate streams. R239's GPU GDeflate capability remains validated,
but no GDeflate claim is made for these native archive bytes.

The CPU still performs metadata lookup, resource creation, scheduling and
submission. DirectStorage may use internal staging. GPU destination does not
mean a physically direct SSD-to-VRAM transaction with no CPU involvement.

## Validation and limits

Windows CI validates eight RGBA/BGRA/BC format variants and all 64 mip contents
against a synthetic compact archive after GPU repacking and copy. It exercises
batch reuse, range/size rejection, duplicate destination rejection, state
restoration and native admission/reference/publication forwarding. CPU
readback exists only in the test executable. Existing rendering checks remain.

The first game run must verify accepted/completed coverage, normal level load,
traversal, texture arrival, unloading and device/error behavior. These cannot
be established by WARP fixtures alone. No frame-rate or starvation improvement
is claimed before live validation. Cube/volume/array textures and in-place
reloads remain native; this is not universal replacement of every game read.
