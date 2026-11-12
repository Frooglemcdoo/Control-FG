# Native write observation — November 12, 2026

## Implemented boundary

The existing `rt_spot_copy_audit` hooks now forward both `CopyBufferRegion` and
`CopyResource` to an independent `gi_reuse::runtime::liveBufferCopy` callback.
Previously those hooks observed resource identities and a selected diagnostic
table, but offered no ordinary-copy callback alongside `liveBuild`/`liveCopy`
(which describe AS build and AS clone/compaction).

The callback preserves the command-list pointer, both resource pointers, offsets
and byte count. Whole-resource copies have an explicit flag and an unspecified
byte count; they are not represented as a zero-length ranged write. Notification
occurs before native recording and does not depend on the legacy inventory's
enabled/fault state or the selected-table capture budget. The native API call
still executes once with its original arguments. No observer is installed by
default; the unregistered path allocates nothing and queries no resources.

This is a recording boundary, **not a completed content-version tracker**.
Arguments are borrowed. A subscriber must validate resource dimensions and ranges,
resolve retained allocation identities, retain the recording generation, account
for Reset/replay, and apply events in actual submission order using the existing
queue graph. No game scene publisher subscribes yet. CPU mappings, UAV/shader
writes, aliasing and descriptor/material remapping still require coverage before
copies or native producers can be removed. Partial hook installation must also
be rejected by any future subscriber claiming complete coverage.

## Native routes checked against supplied binaries

`audit-gi-native-mesh-writes.py` extends the existing hash-pinned audit rather than
replacing it. Seventeen instruction windows are checked; mutating each window is
rejected. Output: `rt-native-buffer-write-routes.json`.

| Entry | Verified route | Consequence |
| --- | --- | --- |
| D3D 26840 | CPU data staged in chunks up to 4 MiB, then 26957 calls ranged-copy helper 26990 | Observe the actual destination copy, not only staging allocation |
| D3D 25570 | 255D6 copies into the temporary buffer; 255EC copies into the final destination | A move produces two writes, not just an allocation change |
| D3D 27CC0 | 27D61 invokes command-list slot 17 with destination/source resources | Whole-resource copy bypasses 26990 and must have separate observation |
| D3D 25C54 / 25E84 | Control type zero maps and unmaps the original resource directly | CPU writes can bypass both GPU copy hooks; protect the write interval before admitting reuse |
| D3D 26010 | Existing verified unlock branches reach 26990 | Retaining a native wrapper does not make its contents immutable |

The renderer inventory still contains the exact same 119 mutation-import sites.
It additionally discovers and checks six direct native traversal sites against
the existing feature hooks and scene-window audit:

| Ray call RVA | Existing classification |
| --- | --- |
| A557E | Glass reflections |
| 129652 | Opaque reflections |
| 12C5EB | Indirect lighting |
| 12D342 | Sun shadow |
| 12D8BA | Ambient occlusion |
| 23F9C4 | Spot shadow |

These are static callsites, not a count of rays, lights, per-frame invocations or
complete indirect-consumer coverage. A GI-only replacement cannot authorize
retiring the shared native scene used by these other routes.

## Validation

Local binary audits pass, including all 17 mutated-window rejections and exact
parity of the previous 119-site inventory. Python syntax and git whitespace
checks pass. Windows source validation is tracked separately below; no gameplay
performance or speckle-removal result is claimed.

The live-only metadata fixture verifies arguments, whole-resource representation,
callback detachment, and delivery after a legacy inventory fault. The existing
WARP copy-hook fixture now checks callback arguments for ranged and whole-resource
copies and still verifies actual GPU destination/source bytes, separate-list
submission order, reset and replay. Both normal and capture configurations run
in the existing full `build-pt-reflections-p0.yml` workflow, which supplies their
generated shader dependencies. The focused `rt-buffer-write-validation.yml` runs
the independent live-callback fixture. This requires no new user capture.

R433 remains on hold. Do not distribute the automatically built package as a
new visual fix; this change connects an ownership-observation prerequisite.

## Submission integration constraint

The existing `GINativeSceneQueues::Before` path skips event processing on COPY
queues, and its unregistered-queue check only sees existing scene events. That
was adequate for its restricted AS journal, not a future buffer-write journal.
A subscriber cannot merely append ordinary copies to the current event vector:
COPY-queue events, exact queue Wait/Signal edges and retained recording ownership
must be processed as well. Use the existing order graph, but extend this admission
boundary before treating copy notifications as version protection. Likewise,
CPU direct-map writes need protection before bytes are exposed to the caller;
observing only Unmap after a write cannot repair a racing GPU reader.

Focused Windows validation passed: run 43294937245, job 131450605434, source
commit aa6f5ab4de160834e2ac8527f0c0f33d7e85b877. The first focused run passed
this fixture but then attempted the full lighting fixture without its generated
shader headers; the workflow was corrected to leave that fixture in the full
shader build. PTLAS CI also passed at the unchanged runtime source parent
05da24bd7166bb178c1e47fad763ad1545e718d3 (run 43294587846); it is still not an
RTX execution result for R433.

Full Windows run 43294937364 / job 131450664900 passed the live lighting WARP
step, including both normal and capture copy-hook fixtures, at source aa6f5ab.
Full run conclusion: SUCCESS, including experimental proxy compilation and all
remaining validation/package steps. No runtime C++ changed after that source
commit; subsequent changes only extend the offline CPU-mapping audit and record
these results. No new user test is requested.
