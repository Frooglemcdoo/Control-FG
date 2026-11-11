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
the NativeDevice object's +88 timeline. 24830 checks the stored fence/value before
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


## Integration continuation

- 43078878208 passed the shared constant-pipeline refactor.
- 43079769047 passed split-recording lifetime cases: producer completion/reset
  cannot retire an unattached future consumer; last-consumer completion AND both
  recording seals are required. Replay, reverse order, cross-queue, failed signal
  and overlap quarantine. Cancellation only applies to an unattached consumer.
- 43079981806 passed actual two-list WARP buffer preservation, transactional
  committed-allocation budgeting and layout validation. A 3840x2160 half-frame
  training buffer is 149,299,200 bytes; weights are 16,588,800 bytes, before guide
  cells and actual allocation alignment. Old 128-MiB scratch pooling cannot be
  assumed suitable. No live pool is connected yet.
- 43080206080 and 43080843343 passed the frozen stage bundle build and extended
  records. A third immutable raygen record explicitly disables guide use for
  native-cosine COMPLEMENT recovery; never retry the complete native ray pass.
- D3D12FrozenCompletion now composes shade-A / guide / trace-B / shade-B, with
  texture/buffer barriers, original ray bindings and final native compute state.
  It requires an explicit separate-producer-submission and scene/descriptor
  lease. No live hook supplies that proof. This recorder is compiled but its
  DXR transaction has NOT executed on software GPU or in Control.
- Full DLL CI initially stopped at the standalone live-toggle test's missing
  d3d12.lib link. The link was added. Headers must be preincluded globally before
  native_lighting scope; isolated tests also retain direct header includes.

R426 pipeline check is distinct from compact-preview image testing. The session
flag CONTROLTOH_GI_FROZEN_PREPARE prepares the new raygen in the same native state
object and attaches six immutable compute pipelines to real GI6, then checks live
binding acceptance. It does NOT record deferred rays, create scene leases,
change RELAX, run a GPU capture, or promise a noise/FPS improvement. The small
load-and-exit result reports ray, compute and binding readiness independently.
Successful shader compile/WARP tests cannot answer RTX-driver DXR integration.
Do not ask for another moving hair/jaw visual comparison for this package.


Further native static audit (renderer SHA ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433):
A8BF0 is a job/queue synchronization join, not itself an AS build. A8C13 calls
waitJob; async path A8C45 waits its scene fence, A8C61 executes deferred thread
cleanup, A8C8F signals the async fence, A8C95 submits graphics, and A8CB4 queues
its wait on the graphics queue. It runs before A8080 binds g_rtScene. Preserve
this native ordering; do not create a competing queue/scheduler.
A3940 (called immediately after GI rays) prepares the three hit textures for
consumption at A3972/A3985/A3998. The observed 12C5F1..12CC1C path then prepares
outputs/providers and GI6. Its imported calls contain no AS build, but this is
not an audit of every transitive callee or concurrently scheduled mutation.
1510B0 is only `mov eax,[rcx+28]; ret`. These narrow findings reduce the remaining
scope; they do not by themselves certify the full native scene lease.


38513c997155786cc98f01c105e9e4557555539c core validation:
run 43082297242 / job 130737667715 PASS, including all CPU/WARP contracts,
complement record identity, bounded working allocation and the new exact-session
pipeline collector. Full DLL run 43082297245 remains the packaging gate.


## Delivered pipeline check

Full DLL run 43082297245 / job 130737708746 SUCCESS. Core run 43082297242 SUCCESS.
Source 38513c997155786cc98f01c105e9e4557555539c. Package 6,937,478 bytes:
`ControlTOH-GI-Pipeline-Check-R426.zip`.
ZIP SHA256 514056f1ea724c15052a547b38f02c52efe0ee61fe0b5d15e95f4d00027060c4.
DLL SHA256 0e194ce7a1313f35a57bb413a438c8e8cd15580cfbbc05bf3b7ba6dc6be3e60b.
All 18 external shaders byte-identical to the delivered compact preview.
Library ID libfile_d78f969f1a488191a467f190e706b916.

Next user input: one load-and-exit result from Start-GI-Pipeline-Check.cmd, with
RT indirect lighting enabled. Automatic GI-Pipeline-R426 ZIP beside launcher.
No movement route, image judgement, performance run, large capture, or repeated
compact-preview test. Need actual RTX state-object and live root-binding result.
Deferred guided dispatch is still disabled. Continue scene/descriptor ownership
integration after reading that result; do not claim this check fixes the noise.

## R426 user pipeline check: logging failure (2026-11-11)

User attachment GI-Pipeline-R426-20261111-131628-57216-eb374b01.zip contains a 3379-byte process-matched probe log and all-false result with no Evidence. The entire log was inspected. It records diagnostics=0 and character pipeline readiness, with no GI readiness/failure lines. This is NOT proof of pipeline failure, wrong installed DLL, or disabled RT.

Confirmed source defect: probe.cpp::Log returns before writing any non-character/non-RT-R411 event when control_diagnostics::Active() is false. R426 incorrectly assumed direct global Log bypassed this gate. GI_FROZEN_R426 and GI_TOGGLE_R308 were suppressed. Missing evidence cannot be recovered from this file.

Fix source b8abcf3a0d39df70734c6ed6dde451398a79f385 admits those exact lifecycle marker tokens through the existing small-event allowlist, adds startup=1/check_revision=2/requested/diagnostics marker, and gives collector separate ready/failed/not_observed statuses. Binding rejected is distinct from absent. Rendering and shaders unchanged. No broad diagnostics enabled. Portable allowlist test passes GCC -Wall -Wextra -Werror; Windows full/collector validation pending. Full run43086394131, core43086394090. Do not ask user to rerun the old package.

### R426 revision 2 ready

Full Windows run 43086394131/job130752099341 SUCCESS; core43086394090/job130751778725 SUCCESS. Includes exact lifecycle allowlist and Windows collector tests (absent evidence distinct from explicit compute failure, process/session matching). Delivered ControlTOH-GI-Pipeline-Check-R426-v2.zip, Library libfile_65be189e31c881918af73336ce95c304, 6,937,711 bytes. ZIP SHA256 faf48618228b520b11ce1a7ea944ef9ac3e32fccb9b48e053f1e3295470a3876; DLL22393b4120f7bdad27877a0b68ac2e5d95433697057e1b05d2bedcad5c67b041. All18 packaged shaders byte-identical to revision1; auxiliary executable differences confined to COFF/debug timestamps (full bytes identical after timestamp normalization). Actual changes mainDLL logging, collector,README. Deferred guided rendering still disabled. Need corrected load/wait10sec/exit RTX check, no visual/movement/performance capture. Second uploaded ZIP ending(1) was byte-identical to original (ff3a89af0029c489030c93555af46e9739526e14bb521a45635b129c881e7ee8).

## RTX pipeline compatibility confirmed, 2026-11-11

Attachment GI-Pipeline-R426-20261111-140015-20272-a5b83376.zip (SHA256 fae193569cdffe32472bc1b0e6f7039f4985528245d3328305e4dcdd0354938e), PID20272, startup19:58:34UTC. Read the entire26-line/4166-byte probe log and cross-checked every collector evidence line against it. Revision2 startup confirms requested=1 and diagnostics=0.

- Six compute stages prepared successfully, HRESULT0, real native root348bytes.
- Frozen raygen prepared in the same state object as native hit/miss.
- Live root binding acceptance succeeded on attempt1, generation415.
- Existing owned ray dispatch at2560x1440, compact_columns=0. Augmented state compact_columns=1 indicates available exports, NOT selected compact dispatch.
- Deferred dispatch remained disabled throughout. No visible noise or performance improvement was tested or demonstrated.

All three required compatibility checks are ready. Do NOT request this check again or repeat the old compact-preview capture. Next integration gate remains ownership of native scene/material/descriptor contents across the two command lists, with bounded per-invocation resources and fence retirement. Merely setting sceneAndDescriptorLease=true in D3D12FrozenCompletion would not establish that ownership. The existing phase capture already supplies ordering metadata; do not repeat it.

Collector packaging observation: PowerShell5.1 serializes Get-Content strings with provider metadata in Evidence (171335-byte result.json). Top-level statuses and raw log are valid and consistent. Normalize Evidence to plain strings in the next packaging update; this is not a reason for another user run. A stripped result containing readiness data only is stored in rtx-pipeline-result.json.

## Shared invocation owner / private queue fence integration

Source ef0c5868c567b4785b0d56c2411197dc4c531728 added GISplitOwnerRetirement; core43093369444/job130775870675 passed. Source07b60e1d8887b15b2074884c06fe3f1f8c379331 adds D3D12SplitOwnerRetirement. Core43093819087/job130777417129 SUCCESS, including actual WARP two-list scratch preservation with owner transfer, private queue fence completion, both successful Reset seals, exact-once destruction and bounded committed byte accounting. Portable policy rejects duplicate/reverse/together submissions, replay, wrong queue and failed signal; pending work pins bounded resources. Same-pointer new generation requires prior successful Reset seal. Adapter Before derives identities from the actual ExecuteCommandLists array using the host generation resolver; Before/After/Poll have no allocations, waits or flushes. Initialization creates one fence outside recording. Host must serialize bookkeeping and provide full Execute/Reset coverage; native scene content ownership is NOT inferred.

GCC -Wall -Wextra -Werror passes; AddressSanitizer/UndefinedBehaviorSanitizer pass with detect_leaks=0 (LSan cannot run under this host's ptrace environment). Do not describe leak scanning as passed. Full Windows run43093819085 is the remaining compile/package check, not a user noise test. These components are not yet attached to native dispatch suppression.

Collector now reads plain strings via File.ReadAllLines: no PowerShell provider metadata in Evidence. Windows tests verify string entries and all status semantics. No user rerun is needed.

### Further native scene evidence

New tools/audit-gi-native-scene-window.py is build-locked to the supplied renderer and emits native-scene-window.json. It verifies exact pdata extent128D20..12DA38, GI ray call12C5EB, shade12CC16, and later native ray calls12D342 (rt_sunshadow) and12D8BA (rt_ao). GI and both later passes read geometry-binding count via A81A0 (scene+88) and pointer via A8190 (scene+80); trivial getters verified byte-for-byte. EarlierA8080 binds TLAS fromscene+C0. This is a promising narrower native ownership window, but both later passes are conditional. Their existence in the binary is NOT proof they execute in this run or that all transitive/concurrent material/geometry mutation is excluded. Do not set sceneAndDescriptorLease=true based only on these observations.

Remaining live work: establish exact native render scope/frame lease and descriptor-content validity before phaseA; integrate shared owner with native queue/reset hooks and preallocated working sets; publish phaseB only with the admitted same-frame scene. Existing R425 phase ordering and R426 hardware pipeline check already passed; no duplicate capture is requested. No hair/jaw quality or FPS improvement yet.

### Recording identity integration and rejected scene shortcut

Actual gi_compute_hooks previously advanced `generation` on BOTH Reset and ClearState. That is appropriate for binding snapshots but not GPU recording lifetime. Sourcef4e2da231ad988043f04ff1c00ad0002dba686da adds independent recordingGeneration, a successful-Reset observer, and a lookup usable from actual Execute arrays. ClearState retains recording identity; failed Reset never seals the old owner. The two-list WARP test now runs these real interposers and checks ClearState, refused Reset, final Reset and private-fence destruction together. CI43095118890(core)/43095118876(full) pending.

Existing hair and jaw R424 logs both show rt_effects=0x6B (107). rr_rt_stack.h maps SunShadow=4 and RTAO=16; both are OFF in those captures. Therefore the later-native-sun/AO-consumer shortcut does not apply to these tested scenes. Do not request a repeat to discover this or force those effects on to create an ownership workaround.

Clarification from exported native identities: D3D136D28 points to the NativeDevice/global device-state object, not a rotating frame object. Its +88 is the native retirement timeline. 2DC50 is DeviceUtil::create and2E240 is destroy; these are the observed setters of136D28.239B0/setCurrentFrame and239C0/getCurrentFrame write/read136C68 independently. Existing prose calling136D28 a frame object was imprecise; do not use pointer stability there as evidence of unchanged scene/frame. Descriptor and deferred-resource retirement stamp the timeline+0 value and timeline+8 fence. The pending proof must tie replay to the actual native timeline/submission, not only the stable device pointer.

### Native Present retirement order and address recycling

Build-locked tools/audit-gi-native-retirement.py verifies17 exact instruction windows and exports against the supplied D3D module. Present31B50 flushes pending native lists at31DE7, signals timeline fence/value at31DFD, increments value at31E73, stamps deferred-release batches at31E76/245C0, then drains with force=false at31E7D/24830. Batch+18 holds the advanced value and fence; ordinary drain249CA..249D9 skips an incomplete batch. The force-drain path is separate. Native queue flush434A0 executes pending lists and stamps each native list wrapper+40 with the current value. This proves the listed native retirement ordering, NOT all geometry/BLAS suballocation/content lifetime. Native-device terminology corrected in R407 notes and variable naming without changing allocator behavior.

Recording identity hardening: use a process-wide saturating serial only at creation/successful Reset. A per-object counter could collide if a destroyed command list's address is recycled while an old owner remains pinned. Added actual-hook WARP coverage that replaces attached State at the same COM address and requires a different identity; serial exhaustion cannot wrap. No per-draw/dispatch atomic added. Windows validation pending.

### Native RT chunk allocation and CI queue stall

The static-provider destructor150AA0 releases primary/scratch/alternate chunks through151720/1518A0/151A20. The skinned-provider destructorA6030 releases its three chunks throughA7220/A7400/A7580. All six decrement chunk+1C reference count and, at zero, remove the chunk and invoke NativeBuffer::releaseResources directly or schedule a cleanup job. Static allocation1504C0/150E21 and skinned allocationA6380 advance chunk+18 and increment references; the inspected frees do not return individual offsets to a free list. Chunk+8 is the native buffer wrapper and+10 the base GPU address. Reproducible build-locked audit/native-pools.json retains the exact function bytes and call identities. This narrows storage reuse concerns; mutable vertex/material/AS contents and precise same-frame consumer integration remain separate.

CI state20:37UTC: old full runs43093369305/43093819085 remain in progress at auxiliary artifact upload; latest recording-hook cores43095118890/43096345342 queued without jobs. Job-log retrieval returned ReadTimeout. Added a3-minute timeout and continue-on-error only to the AUXILIARY preflight diagnostic upload. The actual preflight test, DLL compilation, validation and final package upload still must pass. Added per-workflow/per-branch concurrency cancellation for future superseded runs; it cannot cancel old runs created before this setting. No user action or repeat capture needed for this CI stall.

### Checkpoint20:40UTC, pending Windows validation

Current source/workflow commit f580d0acd020c3337f95db6dcde73ea2d5f16af0 (recording-identity code introduced0e543d624db058aa6a0f0c7b54c4093bcbc5bae3). Latest core43097092781 and full43097092705 are queued without results. The newer ClearState/address-recycling tests have NOT passed yet; only the earlier private-fence/owner version07b60e1 has a successful core run. Native retirement and pool audit scripts passed locally, Python syntax checks passed, changed-file whitespace check passed, and both workflow YAMLs parsed.

Attempted to cancel superseded runs through authenticated GitHub UI because the connector has no cancellation action. Run43093369305 returned Cannot cancel a workflow run that is not in progress while still displaying In progress. Run43093819085 accepted a cancellation REQUEST but remained In progress on re-open. Queued43095118876 returned Failed to cancel workflow. No cancellation completion was verified; do not claim the queue was cleared. Job-log calls also returned ReadTimeout. This is a concrete external validation blocker, not evidence of a shader failure and not a reason for a new game capture.

Next: check latest core/full when runner service progresses; repair any actual compile/test failure. Then finish the same-invocation scene/descriptor borrowing contract and live two-phase dispatch integration. No producer is suppressed, no deferred-guided rays execute, and no new gameplay build is ready. Existing successful RTX check must NOT be repeated.
