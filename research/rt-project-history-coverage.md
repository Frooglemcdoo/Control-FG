# Project history coverage and reuse decision — 2026-11-12

## Decision

Hold the standalone R433 request. Continue source integration using the existing
scene-order, submission and retirement implementations. Queued NVAPI updates
remain hardware-unverified, but that does not block source work on native writers
and consumer routing. Do not ask for generic queue, geometry, material-field,
split-order or basic DXR tests again to establish contracts already covered below.

## Audit scope

Exhausted recursive metadata listings for both project folders: 2,137 files in
Control Epic Compatiable (11 pages), and 1,245 in Control FG mod (7 pages), totaling
3,382 entries. This is a complete inventory of those listings, **not a claim that
all archive contents or every historical run were read**. Results below distinguish
directly read result files, repository analyses, test implementations and CI.
Unopened archives remain unaudited; filenames alone never count as passing runs.

## Reviewed evidence and its limits

| Evidence | What it already establishes | What it does not establish |
| --- | --- | --- |
| October 12 GPU, Path and Geometry result files | RTX 5090 private DXR hits/misses, alpha, finite range, path fixtures and transformed geometry passed | Live Control scene ownership or queued NVAPI updates |
| October 21 Per-Light-DXR-Test.log | Hardware light occlusion, cutout, endpoint, mask, soft edge and composition fixtures passed | Every native light or game consumer covered |
| November 3 GI-Backend-Result.txt | Hardware DXR-to-receiver shading and rejection fixtures passed | Native scene adapter or complete material lifetime |
| GI-Producer-Result.txt and (1) | First result failed because a descriptor heap died before list Close; corrected result passed producer/SBT/hit-field fixtures | Two independent passing runs; spatial/temporal reuse or live scene coverage |
| PTLAS-Validation.txt and (1) | Seven synthetic hardware cases passed | The identical text is not evidence of different test coverage |
| PTLAS-Replay.txt | 48 captured instance snapshots, 6,144 hardware rays passed; NaNs and equal-distance ties accounted for | Mesh geometry was substituted; no native retirement or performance proof |
| R103 source at 2f314e8992aec822f13cafbf7efa8bb55dc3915c | Exact-queue persistent fences, monotonic values, replay/reset ownership and failed-signal retention already implemented | Immutable source bytes or complete writer observation |
| R304/R306 documents and test-rt-as-d3d12-ownership.cpp | Blocked WARP queue retains resources over replay and registry/recording retirement, releases only after fence | Uses empty command lists and ordinary resources, not AS builds or rays |
| test-gi-submission-runtime.cpp | Blocked WARP submission/replay/reset leases and fence-gated release | NVIDIA PTLAS update semantics |
| test-gi-split-owner-retirement.cpp | Consumer admission after actual submission without waiting for GPU completion; generation, replay, failure and release rules | CPU fixture alone is not GPU execution |
| test-gi-frozen-working-set-gpu.cpp | WARP working-set ownership; router section submits producer and consumer without an intermediate CPU wait | Earlier working-set section does wait; router section is not an NVAPI/DXR ray fixture |
| R425/R427 saved capture analyses | Tested split scheduling and descriptor/scene observations already available | CPU ordering is not GPU completion; R427 explicitly does not authorize a scene lease |
| R430/R431 archive audit | Three unique sessions across two bundles; admission/preparation failures and failed gameplay outcome documented | Aggregate held bytes are not copy traffic or physical VRAM exhaustion |
| R433 CI run 43288895526 | Windows compilation and 19 fixture tests passed | No reviewed RTX result for the new queued NVAPI fixture |

Direct result-file provenance (read completely):

- GPU: libfile_650aa2c330d88191a37c16caae36d466
- Path: libfile_f32b0bb7f0888191a07eeddb9f499578
- Geometry: libfile_750b671c5a04819188a5533d89713516
- Per-light: libfile_586bfdaf3bdc8191a5d00c2379969643
- GI backend: libfile_24be524b07208191a005ea4f7b45259e
- GI producer failed/fixed: libfile_ebdd12bd111481918a4f245bf4e124bd / libfile_faa3e54afb888191b0fbdd8934a9575d
- PTLAS validation pair: libfile_30cb384dc8f48191a119ac590fa318eb / libfile_3fe311bffae881919ee06aa4c61a2596
- PTLAS replay: libfile_193410bc359081919f2af3eb86ca4dc5
- RT scene metadata probe: libfile_4fee6d43ff708191ae7bba501198c98d
- R425 collection manifest: libfile_7a981ae62d688191996aec0ec63a9412

## Existing implementation to carry forward

Use `rt_as_ownership.h` and `rt_as_d3d12_ownership.h` for allocation/submission
leases, `gi_reuse/ordered_as_metadata.h` and its existing order graph for version
selection, and the existing split-submission router for actual Execute/Reset
boundaries. R103's `pt_geometry_snapshot.h` and `pt_scene_queue_runtime.h` already
show the exact-queue timeline and hook-order contracts. Do not recreate these
from older per-batch-fence notes. Snapshot pooling is not proof of stable contents:
the R103 implementation explicitly copies fresh source bytes on checkout.

The NVAPI backend path history starts at
4c5fcd047ed5aa6b61a07458046d8fc545bf4e61. Earlier DXR tests cannot be relabeled as
queued NVAPI tests. R433's journal tracks successfully submitted planner revisions;
it still requires the host's existing leases and recording/replay policy.

## Recovered native findings constrain the next implementation

Repository analyses, rather than newly re-read raw logs, provide these findings:

- R225 identifies animated emission and wrinkle fields. Material rebuild sorts
  and deduplicates by hashes, changes output IDs and can split/merge records.
  A per-entry upload to the previous GPU slot is incorrect without mapping updates.
  The regular texture helper already skips unchanged pointers; do not optimize an
  incorrectly assumed unconditional replacement path. See
  `native-cpu/r225-material-update-audit.md`.
- R401 identifies material notifications at entry and provider lifecycle changes.
  Coalesced dirty state and stable pointer/VA are insufficient version proofs.
  Static BLAS compaction switches addresses; transforms, contribution bases and
  BLAS addresses must refresh. Sampled unchanged guards are not completeness proof.
- R403/R404 already narrow CPU-inclusive dynamic recording time to provider updates:
  R404 reports 94.64% in that loop, with common binding below 1 microsecond/job.
  Replacing the common binding helper is not justified as a major optimization.
  These intervals are not GPU times or frame-critical-path measurements.
- R414 supersedes treating the older R391 cost ranking as current. Its incomplete
  phase coverage and observed overlap prohibit turning summed GPU spans into an
  FPS promise. See `rt-ownership-suppression-map.md` for verified figures.

The remaining source task is connecting observed writes to exact allocation
versions and protecting the GPU read interval, including dynamic provider writes,
material remapping and descriptor lifetime. The 119 direct import sites are a
partial static inventory, not complete writer coverage. Existing native consumers
of the shared AS still need complete routing and retirement. The production
replacement-backend pointer remains null with no publishing caller; do not enable
it by declaring the coverage flags complete without implementing those contracts.

No new gameplay fix, performance gain or completed native RT replacement is
claimed by this audit. R433 remains held; no new user input is needed for this
source-level work.
