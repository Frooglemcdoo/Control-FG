# Automatic world-target GPU stage

Working branch: work/gi-auto-target-r448. R448 is not a release package.
Accepted gameplay branch remains work/gi-cost-r440. No game hook includes the new
builder and no automatic targeting is active in normal gameplay.

## Implemented

- `auto_target_contract.h`: 32-byte training records and cells, 64-byte immutable
  output containing two bounds plus source frame/version, sample counts and scores.
- `gi_auto_target_build.hlsl`: Clear, Claim, Accumulate, Freeze stages. World-grid
  cells select two strongest eligible bins. Bounds enclose their entire grid cell.
- `d3d12_auto_target_build.h`: prepared immutable PSOs/root signature, bounded
  recording, buffer/alias checks, UAV barriers and prior compute binding restore.
  Empty input is valid and produces an empty current-frame target list.
- Training energy is quantized/capped for proposal ranking only. It never replaces
  shaded RGB. At most 65,535 records with at most 65,535 points each prevents uint
  score overflow. Hash collisions drop one cell's training; they never merge keys.
- Target identity includes version and source frame. Frame-local material IDs are
  not used as stable identity. Identity fields alone do not solve resource lifetime.

## Validation

Windows run 43896885414 passed initial D3D12 WARP builder tests.
Windows run 43897625033, commit 2f958df8a43a3a130c26112664dc619915e69eb1,
passed expanded tests:

- Actual compute dispatch of all four target stages through the builder recorder.
- Correct strongest-cell ordering, centers, enclosing radii and sample/score sums.
- Nonfinite/negative data and wrong-version samples excluded.
- Empty next frame and new epoch do not retain old targets.
- Colliding hash keys do not contaminate one another.
- 65,535 maximal-score samples accumulate to exactly 4,294,836,225 without wrap.
- Actual GPU execution of the shared pilot resolve agrees with CPU reference,
  including constant fields, adversarial weights, 64 rays and invalid mixtures.
- Exhaustive CPU expectation/positivity tests pass under MSVC.

Tests use the Windows software D3D12 adapter, not the user's RTX 5090. They do not
measure native game rendering, target quality, GPU cost or FPS. The cell-based GPU
learner differs from the CPU clustered-sphere feasibility experiment and needs
captured-data/scene validation; its performance must not be inferred from that model.

## Live binding audit

`gi_live_toggle.h` currently augments the native state object and switches a 32-byte
ray-generation record. Its fixed guide has no external target resource. The
`guidedScopeDraw` thread-local flag pairs the existing callback path but does not
identify or retain a GPU target buffer. A global latest-target pointer would be
unsafe with recorded/in-flight frames.

The new path needs all of the following before it can be enabled:

1. Bounded training output populated from actual freshly shaded secondary samples,
   carrying the matching sampling-density correction. The validated fixture's CPU
   upload is not this runtime data feed. Sparse training is proposal-only; it must
   not omit rays from final illumination or relabel missing training as valid.
2. Immutable target output selected before the ray dispatch and retained until
   that submission completes. Training workspace may be reused only after its own
   completion/ordering is established. No blocking readback or device-wide wait.
3. Ray-generation and evaluator binding of the same output resource and expected
   version/source frame. Pre-sampling invalid-target fallback must select both
   native directions and native weights. Never reinterpret already-guided rays
   as unweighted native rays because setup later fails.
4. DXR root compatibility: adding a private local-root target SRV to an isolated
   raygen export is a candidate; it requires a properly aligned larger shader
   record and explicit association. Existing native hit/miss/global-root bindings
   and original record arguments must remain valid. The private local-root layout
   and 64-byte record encoder now exist and serialize successfully, but native
   state-object augmentation, upload records and actual DXR dispatch remain unimplemented.
5. Matching evaluator root extension and complete native binding restoration.
   Producer/evaluator must reproduce the exact same random coordinates and frozen
   receiver decode. The pilot must be independent of the other nine samples.
6. Submission-scoped resource retirement, exact queue dependency and unsupported
   route fallback. Existing frozen-scene preparation must not be re-enabled; it
   previously caused performance regressions and is unnecessary for these bounds.
7. Actual DXR state-object/record validation and native callback tests, then a full
   Windows proxy build. Only after that is a gameplay package meaningful.

No user capture is required to finish these software integration steps.

## Immutable binding and retirement validation

Commit 7646e93a143c09d8db6bde66ca6a6eca4a451347, Windows run 43904848014:
shader compilation and all Windows validation executables passed.

- `auto_target_binding.h` shares version, source frame, ray count and mixture
  fraction between C++/HLSL. It rejects invalid bounds and incompatible headers
  before sampling. The source frame identifies target construction, not the
  current tracing frame. It limits mixture fraction to the pilot estimator bound.
- `d3d12_auto_target_binding.h` retains proven resource/backing storage and fixes
  selection once. It supplies private space-19 root parameters, a padded 64-byte
  ray-generation record encoder and evaluator root arguments from one owner.
  Space-19 compatibility with native roots still needs explicit runtime admission.
- `test-gi-auto-binding-gpu.cpp` executes the evaluator binding on D3D12 WARP and
  compares its exact controls against the encoded raygen record. Correct targets
  are accepted; mismatched source frame and version are rejected. Local root
  serialization/creation passes. This does NOT execute DXR or link a state object.
- The fixture admits target owners through existing D3D12SnapshotRetirement,
  drops external target references before submission, and verifies that ownership
  cannot retire while submission is pending or the recording remains replayable.
  Successful command-list reset plus observed private-fence completion permits
  retirement. Blocking waits exist only in the fixture, not the binding/runtime.

This is reusable binding infrastructure, not live hook integration. The caller
must still establish immutable GPU contents, queue ordering, resource states,
root compatibility and retention of shader-record uploads/training workspace.
The current class only owns target storage. Neither producer nor evaluator game
hooks include it yet. No gameplay build or new capture is warranted at this point.
