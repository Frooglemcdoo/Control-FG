# Dual-history GI implementation checkpoint

R382 is still the deployed baseline. The user confirmed that it corrected the
weird history artifact, but the 3.87-second `20261108-0101-12.4951726.mp4` still
shows lighting fluctuation. Do not treat intermediate raw GI variance as proof
that the final RAW+RR image flickers.

## Implemented and tested

`src/shaders/gi_dual_history.hlsl` is a standalone candidate temporal core.
It retains native normalized reprojection, the R382 limited receiver-depth
slope correction, queried texture dimensions, per-tap previous-depth rejection,
view-depth offset, and camera-cut handling. It additionally applies slope-aware
current gathering. Native output remains RGB with red duplicated in alpha.

Separate fast RGB/sample-age and luminance moments/long-age sidecars support
short-history bounds and a longer estimate. Temporal variance controls lighting
change response. Fast statistics continue learning while long history reacts;
resetting both on every noisy frame prevented convergence during design.

The candidate is NOT wired into `control_rr_gi::Dispatch`. Existing live PSOs
and the shipped R382 runtime are unchanged. Automatic main-workflow artifacts
from these commits are not new dual-history builds and must not be delivered
as such. The new core is compiled and executed by the feedback workflow only.

Windows D3D12 WARP run 42433358010, job 128586873128:

| Fixture | R382 delta RMS | Candidate delta RMS | Candidate mean |
|---|---:|---:|---:|
| Coherent alternating .2/.6 | .258600 | .006794 | .396823 |
| Same with quarter-pixel reprojection | .258600 | .006794 | .396823 |
| Independent spatial/temporal noise | .023290 | .002431 | .404373 |

Expected mean is .4. Light-off .4 to .02, light-on .4 to 4, a depth-disoccluded
surface .4 to .1, and camera cut all return the new target on the tested
transition frame. A stable 5x5 bright patch retains centre brightness 4.
These are temporal-core tests, not in-game or RR proof, and not a moving/skinned
character test. Full-chain six-pass testing with the new sidecars remains needed.

The ownership policy passes strict Linux C++ and Windows MSVC tests. It pins
history until each reader is submitted, advances last-use retirement fences,
rejects cross-queue reuse, keys recordings by generation/frame/epoch, never
waits on pool exhaustion, and poisons reuse after ambiguous submission failure.
A policy test alone does not establish live D3D12 resource lifetime correctness.

## Runtime integration still required

The current native-compatible Stable replacement binds only the original
resources. The candidate requires t5/t6 and u1/u2 sidecars; these cannot be
assumed populated, nor stored in native alpha. Native root-table capacity is
not evidence of valid descriptors.

Existing reviewed integration machinery offers a route:
- `PTGlass::LightingTableShadow` mirrors CPU descriptor sources, reserves a
  fresh table in the same native heap, and restores root table 0 after dispatch.
- `RRNativeHookGITemporalBind` brackets the exact ConstantTable use at 0x15691;
  temporal dispatch is 0x156d3. Scope observation to this invocation, not all
  shadows or draws.
- `GINativeSceneDescriptors` already supplies CPU copy/constant callbacks.
  A separate temporal subscriber must coexist with the Owned GI producer.
- `GINativeSubmission` provides actual queue submission boundaries. Sidecar
  leases must be tied to command-list generation and actual submission, not
  Present or a presumed number of in-flight frames. Resolve abandoned/reset
  lists and reader retirement before reclaiming buffers.
- The native history fed back after spatial passes must match the sidecar's
  exact source frame, camera/view and resolution epoch. Toggle/reset paths need
  explicit invalidation. Never silently reuse metadata from another condition.

Before enabling: validate the actual temporal root/table ABI, descriptor
restoration, target dimensions, allocation budget, first-frame initialization,
queue ordering, resize/device loss, toggles, and fallback coverage. Add a GPU
adapter test with fence-delayed reuse and all six spatial passes. Then compile
and verify a full new build, including exact packaged shader identity.

Research basis: NVIDIA NRD documentation on fast history, history confidence,
input signal quality and integration; SVGF on temporal luminance variance.
https://github.com/NVIDIA-RTX/NRD
https://research.nvidia.com/publication/2017-07_spatiotemporal-variance-guided-filtering-real-time-reconstruction-path-traced
