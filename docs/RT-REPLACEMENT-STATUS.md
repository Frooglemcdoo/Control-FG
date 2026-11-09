# RT replacement status

Baseline: recovered working source from the R392 build workspace. R392 ZIP
SHA-256: c9c6da09be1c85bf6313ad56153c9a89e61872a2e2f0e3d750f9f4c8b1e050ec.
The recovered source contains uncommitted R387-R392 development; it is preserved
in commit 9ec3155 before these changes. This is not a reproducible-build claim.

## Non-negotiable requirement

Partitioned TLAS / clustered geometry must replace the corresponding native
work. Do not build a parallel scene or keep native preparation active for the
same replaced structures. Conventional BLAS used by a PTLAS are necessary input,
not duplicated BLAS; do not suppress them merely because TLAS is replaced.

Native GI samples, Owned GI, RELAX, 60% clamp and existing quality remain fixed.
The withdrawn sample-toggle change is not part of this work.

## Current implementation

`rt_ptlas_nvapi.h` now implements NVIDIA PTLAS capability admission, prebuild
sizing, owned result/scratch allocation, indirect instance-write upload, actual
PTLAS build/update recording and UAV ordering. It contains no conventional TLAS
build or native retry. It is used ONLY by `tools/ptlas/validate.cpp`; the game
hook's backend pointer remains null.

`rt_ptlas_plan.h` preserves native array-slot InstanceIndex separately from
non-unique InstanceID, emits only changed writes after initialization, explicitly
deactivates removed slots, handles masked and reused slots, and does not commit
discarded plans. Its CPU diff is a validation/reference implementation, not a
claim that Control's existing CPU scene preparation has been replaced.

The standalone Windows GPU validator uses a shared triangle BLAS, first completes
and releases all conventional TLAS references, then tests PTLAS. It checks native
hits against analytic results before comparing PTLAS results, avoiding a false
pass when both paths return misses. It includes movement, masking, destruction,
slot reuse and empty scenes. No native TLAS builds occur in the PTLAS phase.

NVAPI source is pinned to 70d337db9186e968eab622f7e786de7e437faf3d. This path
does not require the new DirectX experimental runtime. Cluster capability is
queried; clustered BLAS construction is still unimplemented.

The standalone validation components are published on GitHub branch
`work/rt-replacement-r392`, based on the remote `work/gi-modernization` tree.
That remote branch is NOT a full publication of the recovered local R392 source;
it builds only the standalone validator, not a game DLL. Local R392 changes and
the disabled native hook remain separately committed in this working repository.

## Disabled game integration foundation

`gi_geometry_probe::BuildHook` previously called the observer and then native
BuildRaytracingAccelerationStructure unconditionally. It now chooses a single
route before either recorder runs. The replacement branch skips the old native
observer/snapshot work as well as the native build. Backend defaults to null.
There is no enabled replacement, additional AS build, or change to R392 rendering.

Admission requires declared coverage of device support, every consumer, copy and
postbuild operations, lifetimes, addresses, and retirement of native preparation.
These booleans are prerequisites for a future backend, not evidence of coverage.
Preparation must issue no GPU commands. Replacement recording cannot return a
failure that falls through to native. Backend ownership must remain consistent
across update/copy/reset/submission; rejecting an update to an already replaced
structure must never feed that structure into a conventional native build.
That persistent ownership machinery is NOT implemented yet.

## Source findings and outstanding work

- `streamline_bridge.h::HookD3D12CreateDevice` creates the native device before
  installing hooks; no cluster/PTLAS capability path is present. The standard
  API needs the matching Agility preview runtime and pre-device experimental
  enablement. Investigate early bootstrap or a supported NVAPI path before
  selecting an implementation. The standalone validator now uses the NVAPI path;
  no game device capability/bootstrap path has been installed.
- `gi_geometry_probe::CopyHook` forwards native AS copies. Replacement cannot be
  enabled until all relevant copies and postbuild queries are handled.
- `gi_native_scene_descriptors.h` tracks native AS descriptors. It does not yet
  remap all native and owned consumers to replacement storage.
- Native destination allocations are sized for native builds. PTLAS requires
  its own prebuild sizing, storage and scratch management; never write PTLAS
  into a native allocation based on an assumed size.
- GPU-generated instance records need stable identities, exact removal handling,
  current transforms/masks/SBT contributions, and queue ordering. Preserve
  instance-index semantics required by existing shaders.
- Cluster-local primitive indices require mapping to original geometry/material
  identities for every shader that consumes them. Existing opaque native shader
  libraries are not automatically cluster-compatible.
- Suppressing the API build alone does not remove upstream CPU preparation.
  The native producer boundary must also be established and retired for replaced
  work. No claim of reduced CPU overhead is made.

## Existing evidence used

Read Performance-R391-25040-116045718.zip, including SUMMARY.txt and GPU stage
timings. The Owned+lighting+RELAX phase reports 1084 Owned GI dispatches and zero
native GI ray dispatches. This demonstrates GI route exclusion for that phase;
it does not measure AS build time or prove PTLAS consumer coverage. Do not request
another general-purpose capture merely to rediscover the existing evidence.

## Validation

`g++ -std=c++17 -Wall -Wextra -Werror tools/test-rt-as-build-route.cpp`
passes: native-only route, replacement-only route, no native retry after an
interrupted replacement, all 64 coverage combinations. `git diff --check` passes.
The instance planner also passes its portable tests, including address/flag
validation, invalid partitions, nonfinite transforms and stale-plan rejection.
ASan/UBSan pass with leak detection disabled because LeakSanitizer is unsupported
under this execution environment's tracing. DXC compiles the validation shader.
The Windows hook translation unit has not been compiled; no local DXR GPU or
in-game validation has run. Windows CI builds the standalone validator and runs
portable tests, but its runner cannot establish NVIDIA GPU results. No game DLL
or FPS gain is claimed. Keep using R392.

## Recovered CPU and lifecycle evidence

Reviewed ten logs from five November 3 capture archives, plus seven recent local
diagnostic archives (R378, R386-R391) and the local November 7 probe logs.
`research/native-cpu/ptlas-existing-log-review.json` records historical file hashes
and final AS observations. Older R304 records explicitly reported
`native_retirement=0 traversal_coverage=0` and missing AS roots. Their successful
metadata matches must not be promoted to complete lifetime/consumer proof.

The matching renderer was recovered (SHA-256
ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433) and the existing
static streaming audit passes against it. Existing R227 evidence attributes
3.223527 ms of a 3.297893 ms sampled preparation job to scene selection/assembly,
versus 0.011580 ms prebuild sizing and 0.062787 ms buffer management/upload. These
are old sampled wall-clock observations, not predicted savings in R392. Native
preparation retirement must address the assembly path; bypassing only the final
TLAS API cannot establish a CPU win.

Primary API reference:
https://devblogs.microsoft.com/directx/clas-ptlas-preview/

## RTX 5090 GPU validation received November 9, 2026

User-returned `research/native-cpu/ptlas-5090-validation.txt` reports driver
62114 (r621_02), cluster query status 0 / caps 1, and PASS for all seven
fixtures: initial, unchanged, rigid move, mask off, destroy/remove, slot reuse,
and empty scene. Final parity result reports six PTLAS builds and zero native
TLAS builds during the PTLAS phase. This confirms the tested PTLAS operations
on the user's GPU. Cluster capability is available, but cluster construction
and traversal are not tested by this executable.

Input SHA-256: `6767c417356b1b6ad966bf3d9bfcceafd8befaa40d285e57998e27f847b2bb00`.

This resolves the standalone GPU validation prerequisite. It does not establish
Control shader/material compatibility, native CPU preparation retirement, or
performance gains. The game replacement backend remains disabled. The next
integration gate is persistent AS ownership across build/update/copy/reset and
all consumers, coupled to retirement of the native scene assembly producer.
No repeat of this synthetic GPU test is required for the unchanged backend.

## Native preparation boundary, November 9 follow-up

Static inspection now traces the preparation callback through job+0x18 to
A81B0, the query receiver at job+0x28, and its vtable method +0x50. The scheduler
publishes the job handle at renderer RVA 0x90E9D8. These offsets are checked
against the exact renderer by `tools/rt-scene-probe/audit-boundary.py`, with
results in `research/native-cpu/rt-scene-query-boundary.json`.

The query's concrete runtime implementation is still unidentified. The prior
historical review and a targeted scan of fourteen currently available log/text
files (including R391 and the returned PTLAS validation) contain no query target.
`rt-scene-query-log-review.json` records that scan. This is the next missing input
for tracing upstream scene membership/change tracking rather than retaining the
native assembly loop behind a PTLAS API swap.

`tools/rt-scene-probe` builds a standalone read-only process reader. It checks the
renderer file hash and live query instruction, then samples bounded job/context
metadata for 30 seconds. It records module-relative query/vtable identities and
observed count ranges. No code injection, AS builds, setting changes, or native
work suppression occur. Matching reads reject obvious races but are not an
atomic snapshot, lifetime proof, or permission to reuse scene data. A missing or
unsupported process is rejected. The game hook remains disabled pending real
producer/consumer ownership integration.


## R394 captured-descriptor parity and ownership implementation

The returned PTLAS-Replay.txt reports all 48 captured snapshots and all 6,144
sampled ray comparisons passing on RTX 5090, driver 62114. It contains 26 NaN
instance records across five snapshots, and 56 independently checked equal-distance
triangle ties. CPU replay reconstructs all 467,638 source records byte-exactly.
The test substitutes an owned triangle BLAS, so it does not establish native
geometry, shader-table execution, live resource ownership, or FPS improvement.
No further run of this unchanged test is needed.

The new rt_as_ownership.h implements a portable ownership ledger. Allocation
identities include a generation; routing is sticky for that allocation. Missing
replacement readiness returns Abort for a replacement-owned structure, never
Native. Native allocations cannot silently convert while references survive.
Closed recordings retain leases for replay; prepared submissions acquire their
leases before ExecuteCommandLists. Executing blocks cancellation, Submitted
permits release only through the exact queue/private-fence timeline, and failed
signaling quarantines references. Native retirement removes lookup eligibility
without invalidating existing recordings or submissions. Address reuse is refused
while an older generation still has a lease. This deliberately conservative alias
policy must be revisited only with an explicit GPU aliasing adapter.

Tests cover replacement refusal, native-only admission, stale generations,
overlapping allocations, two queues, replay, reset/discard, cancellation before
execution, prohibited cancellation after execution begins, signal failure and
closed-list retention after GPU completion. The ledger is NOT connected to Control
hooks yet. Its owners must carry actual COM resources and exact native recording
identities before that integration can establish any runtime coverage flags.
Destroying a ledger requires proven device-idle drain or device teardown.

Feasibility assessment: PTLAS descriptor behavior has passed the tests performed;
there is no demonstrated API blocker. Benefit remains unproven. Existing R227
measurements place most sampled CPU preparation time in scene selection/assembly.
An API-only swap would leave that work. R393 samples also show heavily reordered
slots/SBT contributions; approximately 94.39% match between adjacent samples
when comparing descriptor multisets excluding only SBT contribution, versus
0.73% exact full-record matches and 0.52% exact same-slot matches. Sampling is
500 ms apart, not consecutive frames. Those observations motivate investigating
stable membership and binding updates; they are not predicted FPS savings.
