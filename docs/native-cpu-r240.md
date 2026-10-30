# R240 native texture contract validation

R239 passed file-to-GPU texture validation on the user's NVIDIA hardware:
`compression_support=19 gpu_decompression=1 native_game_integration=0`.
The support mask is hexadecimal 0x19; this run did not use CPU fallback.
That validates the backend, not the game's texture integration.

R240 samples the actual game's texture request, archive read, upload and
publication-queue path. **native_gpu_redirect=0**: native loading still supplies
all game textures. This build is not expected to fix GPU starvation yet.

Run `Start-CPU-R240-TextureContract.cmd` beside Control_DX12.exe after copying
the package contents there. It preserves pinned 8 ms / 16 partitions and the
accepted visuals. CPU speculative read-ahead and demand cache are disabled.
R238 completion polling instrumentation now requires explicit opt-in and is
disabled here. Native completion loops themselves are unchanged.

Run one normal traversal through the affected area, then use
`Collect-ControlFG-Compact-Logs.cmd`. Send the compact log ZIP and observations
about texture arrival and traversal stutter. No Reference run or video is needed.

## What this establishes

The checked renderer request call at RVA 158247 imports runtime
requestFilePartialLoad (FA2B0). Before submission the renderer has allocated
its level container. Constructor observation associates the request ID before
the loader makes the request visible to workers. Sampled native reads supply
archive offset, byte count and file identity. Upload call 15831A and publication
calls 158374 / 1583FE correlate those observations with the texture level.

The 2D mode-zero native upload helper reads tightly packed rows and can load
multiple mips for one streaming level. DirectStorage TEXTURE_REGION requests
require GetCopyableFootprints layout, so the two are not automatically
interchangeable. Logs include each mip's compact offset, row size, row pitch,
compact byte count and independent single-mip footprint size.
`footprint_bytes` summed across mips is a sum of independent requests, not an
aligned multi-subresource file layout.

Look for `TEXTURE_R240 installed=1`, `TEXTURE_R240_REQUEST`,
`TEXTURE_R240_MIP`, `TEXTURE_R240_READ`, `TEXTURE_R240_UPLOAD`,
`TEXTURE_R240_PUBLICATION_QUEUED` and `TEXTURE_R240_STATUS`.
A tight-size match alone does not prove codec or payload identity.
The resource state is an engine-tracker snapshot at request time, not a
GPU synchronization guarantee. Publication queued does not mean GPU-ready.

## Limits and safety boundaries

First eight requests and then every 64th request are sampled, capped at 96.
Only metadata is copied; no payload is exported, decompressed or repacked.
Native pointers are borrowed during their existing request call and only numeric
identities are retained afterward. No resource pointers, states or lifetimes
are changed. Fixed-size correlation tables never overwrite an active entry;
collisions and ambiguous upload identities are dropped. Contended observers
skip rather than wait. Canceled requests may occupy a slot until process exit;
this is bounded and can reduce coverage. Logging and metadata inspection have
some overhead, so this is not a clean performance baseline.

Next integration still requires authoritative I/O ownership, validated source
layout/codec, GPU repacking or prepared GPU-ready assets for incompatible rows,
native queue/state synchronization, cancellation/unload lifetime protection,
and publication only after the GPU transfer completes. GDeflate cannot be
assumed for existing archive data. CPU request metadata and submission remain
necessary; the intended offload is payload movement and eligible decompression.

Windows CI tests argument/return/error forwarding, nested requests, owner
matching, collision rejection, ambiguous uploads, sampling limits, failed read
counts and real WARP GetCopyableFootprints behavior for RGBA/BC7 mips.
