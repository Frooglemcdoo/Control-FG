# R60 PT-on full-frame audit, 15 October 2026

The uploaded labels were reversed, as confirmed by the user and render dimensions.
`PT-On-DLAA-20261015-070645-182.zip` is Quality (2560x1440).
`PT-On-Quality-20261015-071004-167(1).zip` is DLAA (3840x2160).
Both contain build ef607e0e2f1a492f18a5a2527ef7d63a2ee19fda,
PID 53592, startup capture enabled, FG enabled presents zero, and zero ETL
EventsLost/BufferLost. Recording boundaries use Capture.json QPC bounds;
WPR stop/compression and rundown are excluded from measurements.

| Observation | DLAA | Quality |
|---|---:|---:|
| Recording seconds | 10.014623 | 10.011755 |
| Real swapchain presents | 441 | 425 |
| Present cadence FPS | 44.0313 | 42.4839 |
| Mean present interval ms | 22.7111 | 23.5384 |
| Main thread on-CPU ms | 9422.4878 | 9462.3667 |
| Main-thread sampled instructions in mod DLL | 2478 | 2554 |
| Main-thread total CPU samples | 9403 | 9459 |
| Interior queue-derived GPU work union ms | 7023.4699 | 4317.8833 |
| Interior window ms | 9014.6228 | 9011.7550 |

Queue activity is an estimate reconstructed from ordered QueuePacket starts and
stops, treating 244/245 as waits and excluding waits from work, then unioning
overlapping game queue intervals. No unmatched or out-of-order nonwait stops
occur in the interior windows. This is not hardware shader timing. Method:
https://github.com/GameTechDev/PresentMon/blob/main/PresentData/GpuTrace.cpp
Hardware-scheduled queue timing has known accuracy limits; exact GPU cost or
untraced FPS cannot be inferred from these percentages.

PT's own timestamp queries independently show the private pass falling from
about 3.9 ms at DLAA to 1.8 ms at Quality; RR evaluation falls from about 6 ms
to 3 ms. Lower resolution works. The captured frame remains submission-limited.
WPR materially lowers FPS versus normal play; do not present captured FPS as
normal-play performance or use the trace as an FPS acceptance run.

PDB procedure ranges from the matching R60 DLL/PDB resolve the sampled mod
instructions. Direct main-thread samples in Catalog::Scene: 358 DLAA / 416
Quality; Graph::Select: 146 / 169. Inclusive PrepareRecording samples:
1354 / 1351; Revalidate: 489 / 510. In Quality, 1277 PrepareRecording samples
occur outside the estimated GPU work union. Inclusive counts overlap and are
not summed. Stack-cache keys are resolved to their first subsequent deletion
record, not the final occurrence of a reused key. Folded STL symbols are not
attributed to a particular caller without stack context.

## R61 targeted change

Submission currently builds a fresh Scene, compares it to the private prepared
Scene and destroys it. MatchScene now re-selects every dependency using strict
submission ordering, verifies versions/owners/instance bytes and shared geometry
identity, and reuses the retained scene only on an exact match. Changed pointer
identity (including history compaction) falls back to the old full path.
Unresolved waits remain pending; resource owner changes still run full plan
preflight. No GPU checks, queue edges or retirement guarantees are removed.

Resolve also uses a bounded reusable recursion-path vector, removing a tree-node
allocation/free for each selected BLAS while preserving cycle/depth checks.
Local catalog regression passed, including late waits, late BLAS changes under
an unchanged TLAS, masks, modified snapshots, compaction and retained owners.
A 4096-BLAS synthetic check measured 1.416 ms for reconstruction+comparison and
1.067 ms for MatchScene, 500 iterations each. This is a local microbenchmark,
not an in-game FPS claim. Normal-play R61 validation remains required.
