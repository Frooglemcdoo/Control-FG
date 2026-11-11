# R426 binding and lifetime integration checkpoint

No gameplay noise fix or performance result yet. The frozen shaders and recorders
remain disconnected from the live dispatch hook. Do not ask for another compact
preview test. Existing R425 captures already establish the ordered same-queue,
separate-submission schedule; do not repeat that capture.

## Verified GPU components

- Run 43072910936: ray b0/b1 byte preservation and deferred replay after native
  UPLOAD reuse; original compute state restored, full range and padding checked.
- Run 43074271173: actual frozen raygen and generated native-helper GI6 shaders
  compiled with DXC warnings treated as errors.
- Run 43075411386 / job 130713815069: generated evaluator main executed on WARP.
  Lighting helper deliberately stubbed; tests disjoint column coverage, padded
  dispatches, training, weighted hit AND miss radiance, zero-weight contribution,
  unchanged hit distances/alpha. This is not native-lighting parity.
- Run 43076111067 / job 130716227111: production D3D12FrozenEvaluator recorder
  executes the same test, including extended root, full binding validation,
  native root/PSO restoration and invalid-admission rejection.
- Run 43076529635 / job 130717674855: production four-stage guide recorder,
  clear/claim/accumulate/freeze UAV ordering and current frame/epoch output.
- Run 43077414403: SBT DEFAULT and UPLOAD source paths survive native buffer
  overwrite. Native DEFAULT source requires explicit NON_PIXEL_SHADER_RESOURCE
  admission, is transitioned for copy and restored before native use.
- Run 43077926791 / job 130722570517: frozen raygen local-root export association
  and 96-byte raygen records at 128-byte offsets. GPU readback validates both
  phases' identifier, addresses, control bytes and padding. No DXR dispatch on
  WARP is claimed.

The sampler, training CDF, SBT bytes, bindings and constants have separate tests;
none establishes the live geometry/AS lease or the complete split transaction.

## Native SBT correction

Audited exact D3D SHA256:
`cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5`.
Original local Epic module has a different full-file hash, though the inspected
SBT code regions are byte-identical. Recovered the already-uploaded Steam (4)
module and checked the exact hash before retaining this evidence.

- 35701 selects alternating 0x50-byte buffer wrappers, keyed per ray dispatch.
- 35752 releases/grows; 35763 calls allocator 26360 with type 3.
- 26360 allocates a DEFAULT resource for that type, initially COMMON.
- 26517 stores ID3D12Resource* at wrapper+0. 26522 obtains GPU VA; 2652A stores it
  at +48; 2652E stores capacity at +30. No global inventory is needed to acquire
  this resource synchronously at its original native call.
- Writer 33A40's direct path transitions NON_PIXEL_SHADER_RESOURCE -> COPY_DEST,
  copies generated records through native upload machinery, then 3441C restores
  COPY_DEST -> NON_PIXEL_SHADER_RESOURCE. Worker scheduling must still respect
  native submission ordering; a CPU callback alone is not completed GPU data.
- Previous D3D12GISbtSnapshot accepted only UPLOAD and therefore could not admit
  the real native table. It now has an explicit DEFAULT path and tests both.
- AcquireNativeGISbtSource reads only the exact wrapper, validates resource base,
  capacity, buffer shape and DEFAULT heap, then retains that resource. Its caller
  must already establish exact engine build and current invocation identity.

## Scene lifetime: progress, not a completed lease

24430 retains resources in a deferred release queue. 245C0 stamps that queue from
the frame object's +88 timeline. 24830 checks the stored fence/value before
draining it. This supports frame-based retirement but does NOT yet prove every
BLAS, local-root geometry address or material mutation is covered for deferred
GI replay. No new global AS/geometry inventory has been enabled.

Renderer A8080 binds g_rtScene from scene object +C0 (NativeBuffer wrapper).
The caller is 128F5B, before ray passes, with scene object from render owner +220.
ShaderBuffer::setBuffer (D3D 417B0) stores only a CPU descriptor handle, not the
native resource pointer. Do not interpret that registry value as ShaderTexture's
two-pointer representation or use texture offsets to recover the TLAS.

## Sampling detail and correction

Unmodified native raygen uses the older basis. Existing OWNED/compact raygen
already normalizes its normal and uses Duff, matching the new sampler. An early
suspected old-owned/new basis mismatch was ruled out by reading owned_raygen;
do not report it as a cause of existing noise. Both new phases use the same
frozen raygen/receiver implementation as training, avoiding regeneration drift.

Native RNG counter is sys[69].z; host frame/epoch are separate identities.
Training uses the preserved original ray constants, not GI6's differing CBVs.
One configured ray, variant zero, pass zero/count one remain mandatory admission
conditions before native suppression. Guide weight applies to all radiance,
including misses, but not RR hit distance or sample denominator.

Remaining: live same-frame scene/descriptor leases, fence-retired frame allocations; connect trace-A / shade-A / guide / trace-B /
shade-B with native complementary fallback and full publication. Then full DLL
build and an actual moving-scene quality/performance test. No new user input is
needed for the completed component work.

Constant-copy PSO/root preparation has now been split from per-invocation storage;
the live frame owner can reuse one immutable D3D12GIRayConstantPipeline.
