# RT ownership and existing-run audit — November 12, 2026

## Decision

Do not request R432 or repeat R423–R427. Do not advance R431's split,
private-copy guided method. Its ray/shading exclusion is real, but it does not
replace the scene producer, and its private copies and preparation gate remain.
The next implementation prerequisite is versioned geometry/material ownership
and complete scene-consumer routing, before enabling any replacement AS backend.
This audit changes no rendering behavior and claims no performance improvement.

## Existing evidence checked

- R423/R424 source findings already locate large isolated GI contributions before
  RR, dominated by spotlight contributions at the selected secondary hits.
  They do not prove that every visible artifact has that cause. Material keys
  are frame-local, and a single source frame does not measure temporal variance.
- R425/R427 ordering, original writer, native descriptor and retirement evidence
  already establish the tested split schedule. No repeat ordering test is needed.
- R430/R431 compact archives were re-read and CRC checked. The two bundles contain
  only three unique sessions, not five independent runs. The latest R430 session
  stops after eight output-admission failures. R431 logs guided consumers at
  indices 1–4, 600 and 1200, plus eight logged preparation failures with
  HRESULT 0x8007000E. Its maximum logged aggregate retained allocation is
  758,579,200 bytes (723.4375 MiB). This is neither copy traffic nor total VRAM.
  The HRESULT can originate in local preflight/reservation as well as resource
  allocation; physical VRAM exhaustion is not established.
- R414 performance archive was recovered from the existing upload. All 9,098
  raw frame rows and 133 per-stage aggregates reproduce the supplied counts
  and means within CSV rounding. It is globally INCOMPLETE due to focus loss.
  Eleven phases have reported path coverage; the RR return lacks it and the
  final two measurement-overhead controls have no frames. Do not promote this
  to a complete controlled experiment or request a rerun just to rediscover its
  available stage durations.
- PTLAS hardware validation and captured-instance replay already passed their
  documented fixtures on RTX 5090. Neither proves live scene ownership or cost.

Reproduction: `tools/audit-frozen-existing-runs.py` and
`tools/audit-performance-r414.py`. Reports retain archive/member hashes and
scope limitations. R414 Library ID: libfile_6af2059fd6a0819183d61083477fb627.

## R414 changes the historical performance interpretation

| Existing observation | Result | Use and limit |
|---|---:|---|
| Native RR beginning / full RR | 15.0028 / 15.7755 ms per frame | +0.7727 ms observation in this earlier scene, not the R431 regression |
| Lighting raw / lighting plus RELAX | 15.6908 / 15.7654 ms | Only +0.0746 ms cadence difference despite much larger RELAX GPU span; do not equate GPU span to FPS recovery |
| Full RR RELAX total | 1.1279 ms GPU | Includes denoising/conversions; 11 sampled Presents |
| Full RR prepare / denoise / finish | 0.085504 / 0.998513 / 0.034583 ms GPU | Nested breakdown, not extra costs to add to total |
| Full GI rays including visibility / evaluator | 0.391907 / 0.263942 ms GPU | Visibility 0.13017 ms is already inside ray span |
| Full RR evaluation | 3.1640 ms GPU | Present in baseline too; not an enhancement regression |
| Loaded SR evaluation | 0.389419 ms GPU | Loaded SR frame cadence remains 15.1886 ms; no untouched-engine baseline |

The older R391 ranking cannot serve as a current bottleneck diagnosis. R414 has
native GI samples and much smaller feature-phase cadence differences, while
retaining sizable selected GPU spans. This can reflect waits, overlap or a
different limiting stage; it does not by itself identify the limiting CPU task.
R414 predates frozen GI and cannot price its private copies or gating.

## Ownership and suppression boundaries

| Producer | Outputs and consumers | Lifetime / dynamic updates | Existing replacement and safe suppression boundary |
|---|---|---|---|
| Scene query 165520 / traversal 1655C0 | Ordered selected object pointers into preparation | Query also refreshes cell bounds; object predicates, streaming and callbacks remain native | Native selection is retained. Camera/count equality cannot authorize skipping selection. See `research/native-cpu/rt-scene-query-target.md`. |
| Preparation A81B0 and continuation A8800–A8998 | Geometry/material records, instance records and uploads | Provider/LOD/material state is read live; compacted BLAS address and transforms must refresh | Owned assembly/preparation hooks exist, but native sizing/upload continuation and worker stages remain. Persistent prepared-record cache is disabled; do not count it as ownership or a saving. |
| Provider updates and AS worker A89C0 | Deformed geometry, BLAS build/update/compaction, TLAS | Deformed provider update A67B0 dispatches compute; build A6C00. Compactable build 150FA0 and finalize 151040. Submission/join and native retirement govern usage | Four exact renderer AS import sites are inventoried. AS routing supports build and copy replacement, but no production caller publishes `PublishReplacementBackend`; native route remains. Do not suppress conventional BLAS merely to introduce PTLAS. |
| Hit-source geometry and native SBT writer | Index/attribute bytes and shader records read by GI hit/miss shaders | COM retention protects allocation lifetime, not bytes. CPU upload/copy/move and deformation can rewrite storage | R431 snapshots hit sources, SBT and constants at original ray use. `RearmAfterRetirement` reuses allocation capacity, then copies fresh bytes. No cross-frame immutable-content proof exists. |
| Native GI ray call | Material/normal/position hit textures into GI6 | Original writer scope plus native scene, heaps and queue order | R431 replaces the full ray call with disjoint half dispatches. It suppresses the corresponding native call, not scene preparation. Failed pre-admission remains native; post-command failures cannot retry the full ray call. |
| GI6 evaluator | Diffuse GI target, later filtering/composition and RR input | Native base-mip UAV and matching per-batch inputs | Guided completion suppresses GI6. Uniform-complement recovery permits one compatible full shade; partial failed completion suppresses it. This is not guaranteed valid output after partial failure. |
| Reflections and RT shadows | Their native hit/shading signals and subsequent image consumers | Continue depending on native scene/material/descriptor behavior | Existing feature enhancements are not proof of replacement scene compatibility. No all-consumer remap or retirement of their shared native scene is established by GI success. |
| Reconstruction inputs / RR | Guides, motion/depth, radiance and final upscaled output | Animated skin/hair guide generation and replay have independent contracts | A new tracer must preserve the actual surface/opacity/motion contracts. RR evaluation time alone neither locates raw noise nor measures net frame cost. |

Source anchors: `src/gi_geometry_probe.inl`, `src/rt_as_ownership.h`,
`src/rt_as_d3d12_ownership.h`, `src/gi_frozen_scene_contract.h`,
`src/gi_frozen_runtime.h`, `src/gi_reuse/d3d12_frozen_invocation.h`,
`src/gi_reuse/d3d12_native_hit_snapshot.h`, `src/rr_evaluation_entry.h`.
The D3D12 ownership adapter already retains COM resources and submission leases;
it is a foundation, not an enabled game backend. Do not reimplement it from an
older status document which predates that adapter.

## New static writer inventory

The recovered exact renderer SHA is
ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433.
`tools/audit-rt-buffer-writers.py` decodes PE runtime-function ranges and finds
119 direct NativeBuffer mutation-import sites: 40 lock, 53 unlock, 25 copy and
one move. This is a writer-routing inventory, not 119 proven GI writers.
It excludes register-indirect calls, functions outside unwind ranges, other
modules and shader writes. An address outside the preparation function does
not establish execution outside its gate. The known native D3D lock/unlock
path can copy into existing storage: unchanged resource pointer/VA is therefore
insufficient for content reuse.

Persisting geometry requires identifying which allocations each writer updates,
versioning writes before use, handling dynamic/deformed sources, and protecting
the read interval with actual queue dependencies. A generation counter after
recording alone cannot prevent a later overwrite racing a deferred read. No
safe optimization here consists of merely removing copies or increasing the
768 MiB experimental budget.

## Remaining evidence boundary

Existing records suffice to reject another frozen-guide parameter test and to
continue static producer/writer tracing. They do not establish complete writer
coverage or an all-consumer AS remap. These are implementation prerequisites,
not something another screenshot can prove. Current GPU cost of copies and
preparation waits is also absent; R432 does not measure either. No new user run
is requested by this audit, and no new gameplay package is claimed.
