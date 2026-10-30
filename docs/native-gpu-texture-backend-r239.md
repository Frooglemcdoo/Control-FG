# R239 GPU texture transport component

Status: development component, not an installable Control build. Native redirection is not enabled. Do not replace the game's dxgi.dll or interpret this validator as a GPU-utilization fix.

`src/directstorage_gpu_textures.h` submits file data to DEFAULT-heap D3D12 texture subresources. It uses no CPU payload buffers, Map, memcpy or CPU decompression routine. It supports raw and GDeflate streams laid out according to GetCopyableFootprints, with GPU decompression required by default. CPU fallback must be explicitly permitted and its support flags are exposed. A software GPU in CI is not evidence of physical GPU execution on the user's RTX 5090.

There are four owned batches of up to 32 mip requests on one persistent queue. Each batch has one status entry and completion event. The implementation retains sources and destination COM resources until completion; abandoning a batch suppresses publication but does not prematurely release its I/O destinations. Pending overlapping writes and stale generation tokens are rejected. Whole-batch validation occurs before enqueueing. Close refuses while batches are pending. The owner must keep the object alive until all pending tokens retire; do not destroy it or unload a DLL during active I/O.

The caller must guarantee exclusive COMMON-state destination subresources, native texture/listener lifetime, and publication on the correct game thread. COM resource lifetime alone is not engine-object lifetime. Submission/completion methods have one owner thread. Completion events are manual-reset and must not be reset by the caller; they can be registered with the completion worker. Input streams must fit a staging buffer configured to at least the accepted request size; requests larger than 32 MiB are rejected by this component. Queue capacity includes completion markers and exceeds twice the maximum outstanding operation count.

## Automated validation

The Windows workflow compiles against pinned DirectStorage 1.3, then exercises 16 RGBA/BC7 mip payloads, tiny mips, row padding, nonzero archive offsets, raw and compressed streams, multiple batches, explicit CPU-fallback rejection, source retention, deferred discard, overlapping destination rejection, bounds and generation isolation. GPU readback is confined to the validator for byte comparison. Fixture compression happens before submission, not in the runtime payload path. A second process forces CPU decompression and checks that strict policy rejects it. Compatibility mode is used only for validating byte correctness on a CI machine with unavailable GPU decompression.

## Native boundary mapped this iteration

D3D module SHA256: `cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5`.

- NativeTextureContainer::getNativeTexture at EA80 returns the field at +8.
- NativeTextureUtil::lock at 3C6E0 gets the D3D12 resource from NativeTexture+88, calls resource GetDesc, and eventually Map. Existing texture upload maps CPU-accessible backing storage.
- Renderer 154C80 creates one temporary upload texture per mip, fills rows through BaseFile reads, unlocks, and hands the copy to 155090.
- Renderer copy operation 154F20 calls NativeTextureUtil::copyRegion, then destroys its temporary texture container.
- NativeTextureUtil::copyRegion at 3D190 uses resource +88, accesses native state tracking through +50 or inline +40 (state field +20), emits COPY_SOURCE/COPY_DEST transitions when required, issues CopyTextureRegion at 3D511, then restores previous states.

These findings identify why replacing only the resource pointer is unsafe: the native command stream and tracker expect matching resource states and lifetimes. The exact engine submission/fence boundary, completion publication consumer, texture unload retention and source mip layout still need to be established before enabling DirectStorage writes into native resources. The workspace has game binaries but no representative native texture/archive payload, so real-asset layout compatibility has not been byte-validated. There is no claim that arbitrary existing archive bytes are already padded GPU-ready or GDeflate-encoded.

## Remaining integration work

Connect requestFilePartialLoad's texture-specific admission to archive identity and exact mip ranges; choose a GPU repack path or preprocessed GPU-ready streams where native row packing differs. Preserve callback/thread semantics and native resource retention. Coordinate native resource transitions and queue fences, then publish a mip only after successful transport. Separate integration from R238's per-poll diagnostic hooks and CPU read-ahead cache. The target is reduced CPU-induced graphics starvation and traversal stalls, not a synthetic utilization increase.

## RTX hardware validation

The Windows CI runner reported adapter vendor 1414 and compression support 4 (CPU fallback), so CI establishes texture byte correctness, lifecycle behavior and strict-policy rejection, not hardware GPU decompression. For the RTX test, extract the hardware-validation package into a separate folder and run Validate-GPU-Textures-R239.cmd. This invokes --require-gpu, refuses CPU decompression, and saves GPU-Texture-Hardware-R239.log. It does not launch or modify Control. There is no GPU readback in the proposed runtime path; the validator uses readback only to compare the resulting mip bytes.

Further native lifetime findings: renderer canFinishUnload at 157520 rejects mip states 1, 3 and 4 (while allowing 2); unloadLevel at 158AA0 sets state 3 and queues its operation. Completion publication at 160370 dispatches to 158410, directly on the designated thread or through a queued operation otherwise. These state/queue relationships are evidence to preserve, not permission to bypass the native unload protocol. Native GPU command-list submission and actual fence retirement remain to be connected.
