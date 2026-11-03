# Full GI replacement — editable evaluator boundary

## Accepted direction, 2026-11-03

The user explicitly selected full GI replacement, not replacing direct point/spot
lighting or shadows. Green speckles in the latest room are deferred. Preserve the
R308 sampler/native toggle, authored lights, materials, DirectStorage and RR.

R308 PID 63964: capture part SHA256 and all three indexed files validate. The log
records GI_TOGGLE_R308 ready=1 variant=0, and actual recorded new/native/new/native/
new dispatch choices for revisions 0 through 4. RAW BYPASS was requested. The user
reports the inspected areas clean except green speckles in one space. These logs
confirm route selection, not GPU completion or measured image equivalence. No
additional capture is needed to repeat that route validation.

## Implemented source boundary

`generate-gi-evaluator.py` recovers the entire captured GI6 evaluator as HLSL from
SHA256 1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de.
All 674 executable instructions are represented once, in order, with source
addresses. The source keeps the 8x16 thread group, descriptor registers/spaces,
material arrays, point/spot evaluation, shadow atlases and cookies, volume
fallback, ray denominator, depth fade, output alpha and hit-info output.

The earlier PT translator is reused as a starting point. Its PT-specific range
selection, direction overrides, material mip override, emission extraction and
finite/nonnegative return clamp are NOT included. GI loads use the DXBC W mip
component for 2D resources; array loads retain Z as slice and W as mip. Integer
signed comparisons, bit reinterpretation, source modifiers, masked writes and
resource swizzles remain explicit. Nonzero sample-offset extensions are rejected
rather than silently dropped. The checked-in HLSL is reproducible from captured
bytes; unknown hashes fail before output creation.

This reference port is NOT the replacement estimator, is not bound into the game,
and is not claimed bit-identical in GPU execution. DXC recompilation can alter
floating-point optimization. Register initialization is explicit in the port;
possible native undefined inputs and out-of-bounds behavior need GPU comparison.
No new game ZIP or user capture is justified just by successful compilation.

## Next integration work

1. Validate resource-bound reference execution against the captured native
   evaluator, including hit/miss, local lights and volume fallback. Prefer
   existing captured data and automated WARP fixtures. Compile/structural tests
   do not replace image/output comparison.
2. Separate secondary illumination from source-dependent BRDF/throughput in the
   owned evaluator. Supply actual proposal density from the R307 producer;
   retain all-ray normalization and separate miss fallback.
3. Integrate spatial reuse with true accepted geometry, current native alpha-aware
   connection visibility, destination shading and correct normalization. The
   existing geometry/owner/RTXDI adapters remain available. Do not interpret
   smoothed vertex normals as geometric normals or use source-weighted GI RGB
   as reusable secondary radiance.
4. Add temporal reuse after spatial reuse is valid, with disocclusion and RR
   compatibility checks. Keep the native comparison available throughout.

NVIDIA describes ReSTIR GI secondary radiance as non-directional, unlike terms
in this native evaluator. That approximation must be explicit in the replacement,
not obtained by quietly relabeling the current output:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/ShaderAPI-RestirGI.md
DXBC LD mip/slice semantics:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/ld--sm4---asm-

## Validation

Six local tests pass: reproducibility, all-instruction coverage, mip/slice
handling, outputs/normalization, unmodified material sampling and wrong-hash
rejection. DXC compiles the complete shader as cs_6_0, HLSL 2018, with warnings
as errors. Windows foundation CI now includes these checks. This is compiler and
source-contract validation, not a quality or speed result.

Windows foundation run 41693404033 / job 126117527584 passed at remote commit
6ed98f204a2f6cb43e133e1a6475786784a1ddc7. This includes the complete GI evaluator
compile and translation tests plus the existing CPU/WARP foundation suite. The
GI evaluator itself is not executed by those existing WARP adapter tests.

## Captured accounting replay

The optional third output of `generate-gi-evaluator.py` produces
`gi_evaluator_replay.generated.hlsl` directly from translated instructions at
native dwords 5029, 5136, 5143 and 5150. It reuses the existing candidate harness
and `candidate-gpu-fixture.json` (trace SHA256
`5d7992f6fa5ec78066314f35bc647b0565a5c42eb4e360083b5d39d8d1fd9efb`).
The fixture includes selected captured pixels and two synthetic accounting edge
cases; it is not an image-quality sample. Seven additional malformed inputs and
an output guard region exercise the harness's fail-closed and bounds behavior.

Foundation CI executes both the original accounting shader and the extracted
translation on D3D12 WARP, checking captured total RGB, separate hit/miss RGB,
counts and untouched output guards. These are accounting-boundary tests only:
secondary lighting values come from the prior capture rather than execution of
the complete material/local-light/volume evaluator. They do not establish
complete shader parity, RTX 5090 timing, or in-game image quality. No new game
capture is required for this test. Full resource-bound parity remains outstanding.

Local validation: seven translation/replay tests passed; the replay compiled
with DXC cs_6_0, HLSL 2018 and warnings as errors.

Windows foundation run 41694911293 / job 126122766325 passed at remote commit
4d129e1f41fc5150062e8225778927f89f4aaacf. Both baseline and extracted replay
passed nine valid fixtures, seven invalid cases, dispatch guard and D3D12 debug
checks. Maximum captured RGB absolute error was 2.98023e-08 for each. This is
D3D12 WARP accounting validation, not complete evaluator execution.

## Native pipeline compatibility

`gi_reuse/evaluator_reference_pipeline.h` prepares the translated evaluator using
an existing native compute descriptor. It preserves the root signature, node
mask and flags, clears the native cached driver blob, and leaves the caller's
descriptor unchanged. It is not yet called by the game hooks and does not select
or dispatch the replacement. Callers must validate the exact native identity.

`test-gi-evaluator-pipeline.cpp` reuses the observed eight-parameter native root
from the earlier spotlight capture tests, without their added diagnostic root
parameters. It creates both complete native and translated PSOs, checks invalid
input handling and native-cache isolation, and checks the D3D12 debug queue.
This validates pipeline compatibility only; full resource-bound image comparison
is still required. Windows foundation run 41696125622 / job 126127010038
passed at commit dc6adee94f46aa5c62c654ee3e6ad567d479b61d, including both
complete PSOs with the captured native root, cache isolation and clean debug
validation. The first validator build needed an explicit standard string include;
that compile issue was corrected before this successful run.

Integration must handle both ordinary CreateComputePipelineState and cached
LoadComputePipeline routes. The existing pipeline-library hook already documents
and handles this distinction for other shader replacements; reuse it rather than
adding a second library interception mechanism.

## R309 runtime integration

The test launcher opts into `gi_evaluator_runtime.inl`, preparing a translated
alternate only for the hash-pinned GI6 evaluator. Both ordinary and cached native
PSOs reach the existing observer. The native PSO retains its alternate via private
COM data; no global device or PSO ownership is introduced. SetPipelineState and
Reset select the alternate, preserving all root arguments. Failure or unknown
identity retains native execution. The evaluator test reuses the existing last
F10 button for NATIVE / TRANSLATED; the R307 sampler stays enabled in both modes.
The normal R308 sampler launcher still uses its original toggle.

Dispatch logs report actual selected evaluator mode at recording time, separate
from requested UI state. They do not establish GPU completion. No extra shading
pass, new resource capture, AS inventory or readback is added. This is the editable
native evaluator port, not the new GI estimator. Full in-game parity is the next
user-visible check; existing captures cannot execute this newly integrated binary.

R309 validation/delivery: foundation run 41696986671 / job 126130003207 passed,
including actual runtime Prepare/Select/Bind/Reset state tests on WARP and reused
capture accounting. Full mod run 41696986502 / job 126130003685 passed at remote
commit 49096dca1fd9c110f78bda263228e770aba287df (local e062238, tree
60c961f4002ebd1d60a080fef59f5c78dc258820). Artifact 13262940252 contains the
verified 76-entry install ZIP, 4,295,163 bytes, SHA256
5aaa33811017d99ba90f4d97a0bf52245f1ad1c3ed9ebca5f602e9e6d2c2281f.
The packaged DLL contains the R309 evaluator route and differs from R308.
Library delivery: libfile_7dbe0c126f688191be72cdc5a4c4ca71.
In-game visual comparison remains pending; no complete lighting parity claim.

## R309 user run and finite-range estimator work

PID 20276 (capture part SHA256
`d8b185ac91ada31bd3a76e6d05d0baf998962567c4b823c5a7e1a7212ef5843f`):
all three indexed files verified. Evaluator preparation succeeded without fallback;
nine requested switches were followed by matching recorded native/translated
modes. RAW BYPASS was active and the R307 sampler stayed selected. The user asked
whether a difference was expected; no objective full-frame comparison was supplied.
This establishes routing and a reported lack of obvious difference, not measured
pixel equivalence or a new image-quality result. Do not repeat this route test.

`finite_range_proposal.h` implements a continuous proposal model for an owned
cosine sampler whose range variate is independent uniform. It is NOT asserted for
the original native correlated sequence, and does not set producer verification
flags. The R307 hashing/strata contract and true unshifted hit metadata still need
to be connected explicitly before enabling this model in runtime reuse.

For reach probability q(d), the surface proposal is cosine/pi * q_source.
The receiving response carries q_receiver. Source-only shading cancels the two
q factors, retaining the finite-radius hit estimator; moving a sample changes the
ratio q_receiver/q_source. Dropping the receiving q factor would over-brighten
hits near the range limit. Miss fallback stays separate and normalized by all
configured rays. This is only the reach term; primary BRDF, geometric Jacobian,
true hit geometry, direction-dependent secondary shading and fresh connection
visibility remain separate required inputs.

The shared CPU/HLSL implementation is not game-bound. Local tests verify original
sample energy cancellation, destination/source reach ratio, independent numerical
integration of the range variate and hit/miss expected energy, plus invalid inputs.
DXC compiles the validation kernel with warnings as errors. Windows checks are
added to foundation CI. No new user capture or game package is needed for this
algebraic change.

Integration constraint: this marginal reach model must not also reject samples
against a newly drawn receiver TMax; that would count receiver reach twice.
Geometry gates can use the maximum support envelope, while the receiving q term
accounts for randomized range. Likewise, cosine/pi is the aggregate proposal
across the complete equally weighted strata; a single fixed stratum has a
restricted conditional density. Do not label an isolated R307 stratum with the
aggregate proposal without the corresponding mixture estimator.

Windows foundation run 41701703350 / job 126146356159 passed at commit
53d3e0bcb7f3bcbb4e6fbaef381f5a0db61d14ad. The finite-range CPU tests and HLSL
compile run alongside the existing accounting/runtime/geometry suite. No new
runtime estimator or image-quality result is implied by these checks.

## Receiver candidate composition

`reuse_candidate.h` composes the existing geometric connection, finite-range
proposal and initial shading adapters. It uses the mapped source surface PDF,
receiver BRDF*cos and receiver reach probability, preserving the original
all-ray denominator and depth scale. It requires explicit receiver-compatible
radiance verification; source-dependent native GI6 radiance is not automatically
admitted. Misses remain in the separate fallback estimator.

Visibility is an input contract, not an implemented trace: current frame,
connection ID and scene revision must match, with completed/visible/native-alpha
flags exactly set and reserved fields zero. Blocked, pending, stale and unverified
results reject the candidate. The host must supply these values from a real
native-compatible connection trace; synthetic flags are not runtime evidence.
Marginal reach uses the maximum support envelope for geometry gates, avoiding a
second randomized receiver-TMax rejection.

Local C++ tests pass for analytic source identity and moved-receiver Jacobian/range
weights, plus eleven rejection cases and missing producer/radiance contracts.
The shared HLSL wrapper compiles with DXC warnings as errors. These are synthetic
contract tests, not proof of actual alpha visibility, image quality or runtime
reuse. This component is not game-bound; R309 remains the installed test build.

Windows foundation run 41702579307 / job 126149402860 passed at remote commit
cbc549b6d9a45493433c77b820d79d3425965acf (local f6d429a). Receiver candidate
CPU checks and HLSL compilation are included. The actual fresh visibility trace,
producer metadata wiring and runtime reservoir passes remain unimplemented.

## Receiver-directed secondary shading extraction

`generate-gi-secondary.py` separates the complete translated evaluator into
`GIInitializeReceiver(targetPixel, state)` and
`GIEvaluateSecondary(state, sample)`. Material ID, shading normal/UV and secondary
position/UV are explicit inputs. All native volume, material, point, spot, cookie
and shadow-atlas operations remain; the receiver-to-secondary direction is
recomputed from the target receiver registers. This permits destination-specific
secondary shading instead of transporting source-weighted RGB.

The accompanying native reconstruction entry preserves all 674 instruction
labels once. Only the primary validity control flow and three sample loads move
across the function boundary; every other translated instruction is checked
verbatim after register qualification. The input reference text has its own
SHA256 admission guard. Primary throughput, all-ray normalization, depth fade,
hit info and miss fallback remain in their original roles. The shading normal
is not declared a geometric normal or used as PDF evidence.

Local tests pass and DXC compiles the refactored complete shader with warnings
as errors. Compiled code differs from the monolithic version; no bit-exact GPU
or image-equivalence claim is made. Windows foundation creates its PSO against
the captured native root. The runtime still selects the R309 reference shader;
this extraction is not yet a live reservoir pass or a new game package.

Windows foundation run 41707817615 / job 126167685472 passed at remote commit
`a1c094289652cc2c18a9e486339f29ed1ca7f62c` (local `d0d61c5`, tree
`4cb716a6aa01ee1fd231cd655c04e4eee1dde75e`). Logs confirm five extraction tests,
visibility-table preservation tests, and PSO creation for both the monolithic
and refactored evaluator against the captured native root with no debug errors.
Backend compilation run 41707817434 / job 126167685088 passed too. Local table
checks also passed UBSan. No new user hardware run is requested: the trace shader
is unchanged from the supplied successful RTX 5090 test. Full resource-bound
shading parity, runtime producer geometry and reservoir integration remain open.

## Complete initial reservoir batch

`initial_reservoir.h` and `gi_initial_reservoir.hlsl` implement initial weighted
reservoir sampling over the complete configured ray batch. Each surface candidate
has weight `targetDensity / sourcePdf`; selection uses its fraction of the running
weight sum. Final shading multiplies the selected secondary radiance and primary
BRDF*cos by `sumWeights / selectedTarget`, then by depthScale / configuredCount.
Miss fallback is accumulated separately with the same all-ray denominator. Black
hits and misses count toward coverage, including batches containing only either.

Frame/count/depth mismatch, duplicate ray indices, absent geometric evidence,
invalid random values and numerical overflow invalidate the entire result. A
failed or incomplete result must leave the local estimator available. The GPU
buffer keeps the selected unweighted sample and geometric normal separately from
its reconstructed contribution. This is an original full-precision initial RIS
implementation, not RTXDI packed storage or SDK reservoir merging. The output
field named inverseSourcePdf is deliberately zero in its reconstruction result;
the RIS weight must never be relabeled as a source proposal inverse.

CPU checks integrate 65,536 selections of colored candidates with unequal PDFs
and verify expected energy, miss fallback, 64-ray coverage and rejection cases.
Local CPU and UBSan checks pass; DXC compiles the batch shader with warnings as
errors. The Windows WARP fixture executes 1,024 such batches and malformed inputs,
checks actual selected sample/reconstruction and output guards. This reuses the
existing GPU harness and does not need another user capture. It does not validate
the game's missing producer geometry or enable spatial/temporal reuse.

This stage follows the initial reconstruction-before-reuse sequence described by
NVIDIA: https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md
The game's direction-dependent secondary shading and finite-radius proposal
must remain explicit when implementing later reservoir transport; this initial
batch does not establish spatial MIS or visibility-correct reservoir merging.

Windows foundation run 41708831805 / job 126171222290 passed at remote commit
`48568c357e731d2448756777a40fb4cd940e597b` (local `85b82fb`, tree
`73c6acbe38a8214b1a23cb8ddd16c2e9e6c69fb4`). Logs confirm the CPU expectation
checks and actual WARP execution of 1,024 initial-reservoir batches, selection,
reconstruction, miss fallback, malformed inputs and output guard all passed.
This is GPU execution of the initial reservoir kernel, not in-game reuse quality.

## Producer-to-shading connection

The extracted evaluator now has a world-position entry, used by
`gi_producer_shading.hlsl`. It retains exact native instructions for the existing
texture entry. The world entry supplies the unbiased accepted point directly to
world-space lighting/direction evaluation; it also computes view position using
native cb0[2..5] for light-cell indexing and hit-distance accounting. UV order is
unchanged (position.w=V, normal.w=U). This is not a replacement of authored lights.

The shading pass accepts frame/scene-matched 160-byte producer requests and emits
the same 112-byte sample ABI now shared with the initial reservoir kernel. It
evaluates secondary illumination at each requested receiver, preserves distinct
geometric/shading normals, and keeps misses as primary-weighted fallback rather
than reusable surface radiance. Invalid/stale/incomplete inputs publish an invalid
zero record, which invalidates the whole reservoir batch instead of dropping its
normalization denominator. Request proposal/BRDF values must come from the owned
producer contract; this pass does not infer them from captured weighted RGB.

Resources t27/u2/b5 occupy unused slots already covered by the captured native
root signature. Windows validation creates this PSO against that root. No root
expansion or material-table copy is required just to compile this shading pass.
The descriptor contents and their lifetime still must be bound by the runtime.
Shader compilation, source invariants and ABI checks pass locally. The pass is
not selected in Control yet: native alpha linkage, admitted live scene mapping,
producer dispatch/resource ownership, and spatial/temporal reuse remain open.

Windows foundation run 41711803133 / job 126181476372 passed at commit
`c5ede2970c1ecc07acaf38b7fb20404a98a54b49`. The log explicitly confirms producer
shading PSO creation against the captured native root (t27/u2/b5), alongside the
initial-reservoir GPU checks. This proves resource-layout compatibility, not a
resource-bound full-scene dispatch. R309 remains the last user-facing game build.

## Owned producer traversal and table preparation

The owned producer now has a ray-generation entry (`gi_producer_trace.hlsl`)
connected to its accepted-geometry and recovered alpha-test exports. It consumes
explicit current-frame cosine proposals, uses the native mask/flags and two-row
geometry multiplier, and emits `GIProducerShadingRequest` records. The finite
range draw is independent of direction; accepted hits receive both the marginal
surface PDF and receiver reach factor. Misses retain the true range endpoint for
native fallback shading. Invalid proposals/geometry produce no verified request.
The proposal generator must still establish the complete equal-strata contract;
merely setting `proposalVerified` is not evidence of that contract.

`D3D12ProducerPipeline` prepares a separate state object with a 96-byte payload
and the recovered private local-root ABI. It uses the admitted GI6 global root:
spare t27/t28/u2/b5 slots carry proposals, TLAS, accepted requests and constants.
Those slots require a private descriptor table, not changes to Control's table.
The raygen/miss records and opaque/alpha identifiers belong to this new state.

`PrepareGIProducerTable` converts only active even native records. Reserved odd
rows are zeroed without compacting the table or changing TLAS contributions.
R16/R32 backing bounds and distinct material bindings on shared geometry remain
validated. `D3D12ProducerTable` retains admitted geometry resources/backing heaps
and owns immutable uploaded records; it does not establish scene provenance,
alias activation, residency or submission completion. The host must retain this
owner through the existing submission retirement path before dispatch.

Backing-buffer containment uses a sorted batch index and two prefix maxima for
logarithmic lookup, including nested alias rejection. It avoids repeated COM
queries and scanning the complete resource set for every geometry view. Local
randomized tests compare 262,144 queries against exhaustive containment. Sparse
shader-table conversion, transactional rejection, existing alpha-policy audit
and secondary extraction tests pass locally. The combined ray library compiles
with DXC warnings as errors. Windows compilation remains a separate check.

These components are not yet selected by the game hooks. No new game package or
quality improvement is claimed. Live proposal generation, matched scene/binding
admission, dispatch/retirement, output integration and spatial/temporal reuse are
still required for the complete replacement. Reuse existing PT evidence and
captures for those steps; do not ask for a repeat R309 route test.

The proposal compute pass now produces a bounded tile of complete per-pixel ray
batches directly from the extracted receiver state. It reuses R307's hash/strata
coordinates and adds a separate reservoir-selection dimension. Native primary
throughput is multiplied by the aggregate directional PDF; the accepted-hit
trace applies range reach separately. An owned orthonormal basis normalizes the
recovered near-unit normal to keep that PDF meaningful. This is not bit-exact
native ray-direction parity.

The proposal origin preserves the ray producer's .002*depth normal offset rather
than mistakenly using the evaluator receiver's .001*depth offset. The host must
supply the base range from the matched native producer constants and match the
frame, scene, pixel dimensions and complete ray count. GI6's first-material
validity gate is still used by the extracted receiver initializer, so this stage
still depends on that native output until the full producer route replaces it.

Local shader compilation and R307 coordinate/strata tests pass (including range
and reservoir-selection dimensions and progression beyond frame 256). Windows
validation exposed a const-qualified export-name-array incompatibility in the
new state-object descriptor; the array declaration is corrected. Neither the
proposal compute pass nor private DXR pipeline has been executed in-game yet.

Windows validation completed successfully at remote commit
`82d3095e7bdec89ff3361468e2699ebf41895f1f` (local `5851d36`, tree
`e3952adb7dba0759c03a5ec2f0172f13aec02821`), run 41717035123 / job
126199625970. The WARP fixture executes the proposal shader against synthetic
native receiver inputs and checks reconstructed origin (including the .002 vs
.001 normal-bias distinction), unit directions/cosine, primary BRDF/PDF
cancellation, complete batches, range/selection random values, stale/malformed
controls and output guards. The fixture has no D3D12 debug errors. Existing
initial-reservoir GPU execution and the rest of the foundation suite also pass.
This is synthetic producer validation, not live scene traversal or image parity.

Next integration must establish the current TLAS/BLAS-to-private-SBT mapping and
retain that exact recording's backing resources, then connect proposal, trace,
shading, reservoir and output passes with resource barriers and submission
retirement. The recovered PT descriptor-shadow/native-allocation path can keep
material heaps and root bindings unchanged. Do not copy descriptors from the
shader-visible heap, infer geometry from index address alone, or promote the
metadata diagnostic inventory into a complete scene lease. The proposal origin
and evaluator receiver origin differ; future reconnection PDF/Jacobian geometry
must use the actual source/target proposal origins consistently. Spatial and
temporal reuse remain unimplemented. R309 remains the last game-ready package.

## Recovery and scene table integration, 2026-11-03

The restored workspace initially contained the older R304 checkout. Work resumed
in a separate checkout from remote commit `5b134e934b00ce053279b795a9a5a02c35c9abd5`,
preserving the original checkout. That commit contains the four-stage command
recorder, native binding restoration and bounded-tile descriptor publication.

`scene_producer_table.h` now connects the complete ordered TLAS instance array
to exact BLAS geometry witnesses and the matched native GI writer table. It
reuses R103's `PTOwnedScene::Index::ResolveExact`, imported from commit
`2f314e8992aec822f13cafbf7efa8bb55dc3915c`. Geometry allocation tokens must be
captured at build time; a later lookup by index address is insufficient.
Unreferenced native rows remain zero without compacting instance contributions.
The GPU table owner checks bound flags, reserved rows and backing resource ranges.

Local GCC C++17 warnings-as-errors and UBSan checks pass for producer-table and
scene-hit-packet tests. Coverage includes both index widths, stale revisions and
allocation tokens, duplicate witnesses, shared indices with different position
buffers, distinct per-instance materials, conflicting SBT rows, and transactional
rejection. These are CPU binding checks, not live DXR traversal validation.

The runtime still needs to supply current instance bytes and build-time geometry
identities, acquire the returned resource leases under the owner lock, and retain
the complete AS closure through submission completion. The helper does not make
the diagnostic metadata inventory a live scene provider. Final output integration
and spatial/temporal reuse remain outstanding. No new capture or game package is
requested by this checkpoint.

Windows validation passed at remote commit
`d50fd79ad1f2cd2a51a1ff8c86c0c642ea26e2ac` (local `6ce6fae`).
Foundation run `41722234369` / job `126217744699` passed shader compilation,
scene/producer binding tests and the existing D3D12 WARP suite. Full mod build
run `41722234446` / job `126217746104` also completed successfully. The first
Windows attempt exposed ambiguous unqualified `Bytes` lookup in MSVC; explicitly
qualifying `PTShaderTableAdapter::Bytes` fixed it, and the full foundation suite
was rerun successfully. The mod package still uses the R309 live renderer;
successful compilation does not make the new GI producer live or game-ready.
