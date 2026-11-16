# R448 automatic world-target GI integration

Working branch: `work/gi-auto-target-r448`. Accepted branch `work/gi-cost-r440`
is unchanged. R448 is an opt-in integration test, not a verified visual fix.
The complete Windows proxy/package build is being validated.

## Implemented path

- Native GI6 identification retains the exact shader size/hash admission.
  Its root signature is extended in private space 19, with collision and
  64-DWORD budget rejection. Native parameters, samplers and bindings persist.
- The variant-zero native DXR state object gains a private-local-root raygen
  export. Native hit/miss identifiers and shader tables stay in that same state
  object. A 64-byte immutable record contains the target address and selection.
- The verified native render callback pairs the producer and evaluator. Runtime
  admission requires ten rays, batch zero, one pass and a prepared matching root.
  The small provider interface is initialized independently of scene inventory.
- One independent cosine reference ray and nine guided draws use one shared
  sampler. Complete secondary RGB, including miss/volume lighting, uses the
  pilot resolver. Native range hashes, fade and RR distance accumulation remain
  separate from weighting. Invalid target headers select the prior ten-ray
  owned strata/permutation before tracing; no post-hit unweighted fallback.
- Existing R442 combined structured-buffer loads are applied to the new lighting
  shader. The rejected point-shadow gate is not enabled by this build.
- One rotating receiver/ray per tile produces bounded density-corrected training
  entries. Invalid receivers and misses clear their entries. Invalid ray receivers
  clear all ten material slices rather than leave earlier hit data in use.
- Clear/Claim/Accumulate/Freeze selects two strongest eligible world-grid cells.
  Score quantization affects proposal ranking only, never final radiance.
  Hash collisions may discard training but cannot merge unrelated cell keys.
- Each invocation retains its target, shader record, training, cell and output
  buffers. Existing split-submission retirement tracks the actual native queue,
  Execute, Reset/destruction and private-fence completion. Completed outputs are
  published only after both recordings retire; targets are never overwritten.
  No deferred native scene replay, old radiance blending, blocking readback,
  device-wide wait or legacy scene inventory is introduced.
- Default launch leaves this route disabled. The R448 launcher sets
  `CONTROLTOH_GI_AUTO=1`. GI FIXED enables the candidate at ten rays; native
  diagnostics suppress new auto admissions. Capture movement remains Alt+arrows.

## Failure handling and limits

Unsupported preflight conditions preserve the prior path. After guided rays
exist, the runtime never intentionally routes them into an unweighted evaluator.
A failed paired pass can be suppressed and halts later auto admissions for the
session; this can leave a visible bad frame and is an experimental-build limit.
The log records `GI_AUTO_R448 halted=1` and the reason. Restarting normally disables
this candidate. Partial or uncertain submissions retain bounded owners rather
than release resources still potentially referenced by the GPU.

The queue/recording contract currently requires separate producer and consumer
submissions. New resources are allocated per admitted invocation, with at most
eight retained invocations and a 64-MiB logical allocation budget. This is not
final performance tuning. Feedback can lag by multiple completed frames; stale
bounds preserve expected radiance through density correction but can increase
variance temporarily. The two-cell learner is not the earlier CPU clustering
experiment, and its measured synthetic benefits cannot be transferred to Control.

Hardware DXR execution, live callback admission, visual improvement and RTX FPS
remain in-game validation items. Software tests do not establish those claims.
The final performance goal remains a 1–2 FPS loss, not an early hard cap.

## Completed validation

Windows run 43897625033: actual D3D12 WARP execution of all four target stages;
score ordering and enclosing bounds; invalid training exclusion; empty-frame and
epoch expiry; hash collision separation; exact maximal uint score accumulation;
GPU pilot-resolve versus CPU agreement; exhaustive CPU expectation/positivity.

Windows run 43904848014: immutable target binding; private local-root creation;
64-byte record layout; matching GPU evaluator selection; stale frame/version
rejection; ownership retained through submission and replay eligibility, then
released after Reset and private-fence completion. DXR dispatch was not executed.

Windows run 43906929079: raygen/evaluator compilation and shared sampling/resolve
execution for six states x 32,768 receivers (1,966,080 directions). Unit directions,
finite bounded weights, constant fields, distinct rays and unchanged range draws
passed. Known cosine expectations versus observed scenario means:

| Scenario | E[z squared], expected .5 | P[z > .8], expected .36 |
| --- | --- | --- |
| Front-facing targets | .500130 | .361028 |
| Targets behind surface | .500009 | .360823 |
| Empty | .499995 | .359714 |
| Stale frame | .499995 | .359714 |
| Second target only | .500029 | .360897 |
| Nonfinite bound | .499995 | .359714 |

Windows run 43910019650: actual complete evaluator PSO creation with extended root;
private-space overlap and DWORD overflow rejected; raygen association layout;
irregular tile coverage and padding guard; miss/invalid-receiver training clears;
density-corrected energy. This is not execution of native game lighting resources.

Focused Windows validation also passed on runtime commit eac036c0b3f6dc25ba67b5dbd2d73d5e4be31de1
(run 43912342363). The R447 capture contains no negative complete-RGB components;
recorded range was 0 through 2.3738811. This supports the resolver input assumption
for that capture only. Local inspection of R442 optimization on the new evaluator
coalesces 111 scalar buffer reads to 41 while preserving audited field addresses.
