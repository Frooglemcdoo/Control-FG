# Performance pivot: evidence audit

Date: 2026-11-10. Goal: retain the accepted visual improvements while recovering
the user-reported 20+ FPS loss. CPU ownership experiments are paused. This audit
makes no rendering changes and does not claim a recovered frame-rate gain.

## Decision

The best existing feature-isolation evidence prioritizes the **GI denoising / RR
pipeline**, especially RELAX, ahead of another CPU orchestration rewrite. This
is a priority for investigation, not proof that removing RELAX preserves quality
or that its cost explains the whole current regression.

A trustworthy current vanilla-to-overhaul cost ledger does not yet exist. The
usable historical comparison kept mod RR enabled in every phase, used four GI
samples in enhanced phases, and had glass enhancements disabled. It must not be
presented as today's full visual-overhaul cost or a vanilla-engine comparison.

## Evidence inspected and quality gates

Read all text/CSV/settings members of five saved PERF COMPARE archives, R387–R391.
Recomputed every phase's frame count and mean from frame-times.csv and every
reported GPU stage's count and mean from gpu-timings.csv, grouping by Present
before averaging. Counts match exactly; means agree within 0.0001 ms (CSV
rounding). ZIP CRCs and SHA-256 provenance are recorded in existing-comparisons.json.

Also read all 17 locally available RT-Assembly collector archives (17 text members,
1,328,228 bytes), reviewed the earlier CPU analyses, the R411 ETL analysis, and the
current R413 source/configuration. The CPU collectors do not contain per-feature
GPU stage summaries. They cannot supply the missing visual cost ledger.

| Capture | Coverage / drift | Use |
|---|---|---|
| R387 | Three incomplete feature phases, skipped GPU samples; baseline drift 8.52% | Reject full causal ranking despite its COMPLETE label |
| R388 | Three incomplete feature phases | Partial historical evidence only |
| R389 | Three incomplete feature phases | Partial historical evidence only |
| R390 | Full phase missing required coverage | Partial historical evidence only |
| R391 | All nine phases covered, zero skipped samples, zero profiler fault/dropped rows; baseline drift +0.785%, full repeat -0.695% | Usable historical comparison |

R391 source: [Performance-R391-25040-116045718.zip](https://chatgpt.com/library?file_id=libfile_d7b7dd97a25c8191bc46d623db29b88b&account_id=personal).

## What R391 actually measured

Resolution: **2560×1440 input to 3840×2160 output**. FG off, RR on, clamp 60%,
reflection sample policy Native, DirectStorage requested on, glass enhancement
off. Runtime GI sample masks confirm one sample in the native phases and four
in the enhanced phases. These are this capture's effective counts, not a rule
that native always means one. Legacy PathTracing INI entries do not establish
which experimental path executed; use runtime coverage, not those entries alone.

Each phase used five seconds of warmup and twelve seconds of measurement. GPU
queries sampled every 64 Presents; R391 stage populations are 16–22 sampled
Presents per phase. Frame intervals measure application Present cadence with FG
off, including waits. Selected GPU spans are not whole-frame GPU time.

| R391 condition | Mean frame ms | FPS | Increment versus preceding row |
|---|---:|---:|---:|
| Native GI / mod RR baseline | 8.390 | 119.19 | — |
| Native GI with four samples | 9.510 | 105.15 | +1.120 ms |
| Owned GI, four samples, raw | 9.543 | 104.79 | +0.033 ms |
| Add enhanced lighting policy | 9.912 | 100.89 | +0.368 ms |
| Add RELAX | 11.080 | 90.25 | +1.168 ms |
| Add character filtering | 11.156 | 89.64 | +0.076 ms |
| Add volume filters (full) | 11.346 | 88.14 | +0.190 ms |
| Full repeat | 11.267 | 88.75 | — |
| Native repeat | 8.456 | 118.26 | — |

Full versus initial native is **+2.956 ms**, approximately 31 FPS in that historical
scene. The paired repeat difference is +2.811 ms. The rows are sequential,
conditional changes: their arithmetic telescopes, but they are not independent
feature budgets or interchangeable savings. Small differences near drift/noise
are especially weak evidence. No confidence interval or same-scene motion replay
is available.

## Ranked investigation priorities

| Priority | Evidence | Interpretation / limit |
|---|---|---|
| 1. RELAX processing and its integration with RR | Adding RELAX: +1.168 ms frame time. Full GPU RELAX including conversions: 1.042 ms; receiver export: 0.040 ms. Full CPU RELAX recording: 0.147 ms/call; receiver recording: 0.013 ms/call. | Largest retained incremental cost supported by this historical comparison. Split internal denoising from conversion/export work before optimizing. CPU and GPU values must not be added as a frame-time cost. |
| 2. RR integration and its full GPU span | RR evaluation is 3.119 ms in initial native, 3.173 ms in full, 3.186 ms in final native. Full CPU evaluation call is 0.232 ms. | Large absolute stage, but present in the baseline too. Its full duration is NOT a measured enhancement regression or recoverable saving. Need matched SR/RR control and confirmation of exactly what the span encloses. |
| 3. Enhanced lighting and volume/character additions | Conditional frame deltas: lighting +0.368 ms, volumes +0.190 ms, character +0.076 ms. | Smaller leads in this scene. Lighting policy changes several paths; the entire delta cannot be assigned to the selected shadow-ray span. No isolated volume/character GPU spans here. |
| 4. CPU preparation / provider updates / synchronization | R404: 2.372 ms per preparation loop; recording provider update interval 0.802 ms per sampled job. R411 slow sections have more preparation/join samples. | Historical workloads, diagnostics and scopes differ. These are not additive frame costs, current regression attribution, or a demonstrated benefit from replacing the scheduler. Keep as a separate critical-path question. |

Owned GI temporal visibility is a real additional pass (~0.145 ms GPU in R391 full),
but it is **already included** in the GI-ray span (~0.835 ms). Do not count it twice.
The owned producer plus reuse changed frame time by only +0.033 ms versus native
four-sample GI in this comparison; that small net result is not evidence that
visibility itself is free or that the same result holds at native sample counts.

RELAX and RR both run, but process different parts of the signal pipeline. This
is not proof of duplicate GI ray generation. Removing a denoiser without the
accepted motion, shadow, material and light-removal quality tests is not an
acceptable optimization.

## Costs already changed: do not count them again

- Four-sample GI added +1.120 ms in R391. R392 changed Owned GI back to the
  game-supplied sample count; the user accepted native samples. That historical
  cost is not a fresh optimization opportunity. Subtracting it from the old full
  total would be an estimate, not a measurement of the current full configuration.
- R386's report of two committed upload-buffer allocations per recording is
  historical. R388 added fence/reset-safe reuse. Current nrd_diffuse_pipeline.h
  allocates its buffer only when absent; nrd_diffuse_d3d12.h allocates on absence
  or insufficient capacity. R391's measured RELAX phases show zero newly created
  work bundles, all recorded bundles reused, and zero RELAX rejections. The old
  approximately 1.1 ms CPU recording figure is not today's known cost.
- R410 descriptor tracking is disabled in clean R411–R413 builds; periodic RT
  instrumentation is compiled out. This does not declare all mod overhead zero.
- Native scene selection is the accepted default after R412. It showed similar
  overall FPS and somewhat better lows; no numerical matched dataset was supplied.

## Current configuration and what is not known

R413 source/package permits two preparation modes. Both have native selection,
periodic RT diagnostics off, and the owned recording/binding replacements still
installed. Root DLL defaults to owned preparation; NativePrep omits both outer-loop
and assembly replacements. The user reports both modes hurt performance. Neither
is an untouched-engine reference; no mode is accepted as a performance upgrade.
Do not silently call one of them the performance baseline or commission more
isolated ownership builds based on their install success.

RR bundle defaults still request Owned GI, native GI samples, native reflection
sample policy and clamp 60. Persisted menu settings and live activation determine
other features. We lack a current settings export / phase ledger proving the exact
accepted full configuration. In particular, glass was off in R391, so that capture
does not quantify the cost of every visual improvement the user has accepted.

The R411 traversal recording is a different workload from stationary R391. Its
main-thread Windows message waits dominate three very large hitches, while CPU
preparation/join work also grows in sustained slow sections. ETW stack collection
adds work; the user also observed unrecorded dips. Steam Overlay was already off.
No causal overlay attribution, recording-only explanation, or spike fix is proven.
R412's spike-free runs do not explain the earlier hitches. GPU busy or CPU wait
values from that traversal cannot be joined to R391's stages into one cost ledger.

## The precise missing comparison

One measurement campaign must distinguish these boundaries, with FG off and
matched game binaries, save/camera, native RT settings, resolution, DLSS quality,
VSync/cap and lighting state:

1. Untouched rendering reference with native SR and no injected mod. This is a
   separate launch; a loaded proxy's native mode cannot stand in for it. Record
   game/runtime hashes so different storefronts or DLSS runtimes are not conflated.
2. Mod loaded, enhancements off, native SR: reveals residual mod overhead.
3. Same mod configuration with RR on and other enhancements off: measures the net
   RR integration change relative to SR. RR runtime/API time alone is insufficient.
4. Accepted full visual configuration at native GI samples, with its active CPU
   replacement configuration explicitly recorded.
5. Full configuration with targeted diagnostic ablations, beginning with RELAX;
   restore accepted quality afterward. Split RELAX export, conversion and internal
   passes with bounded GPU timestamps. Repeat reference/full conditions to detect
   drift. Include a fixed traversal segment separately from the stationary scene.

Existing PERF COMPARE supplies much of the feature matrix and restores settings,
but it holds RR on, keeps the same proxy/hooks loaded, and lacks total-frame GPU
and detailed RELAX internal timings. Running it unchanged cannot fill all gaps.
R391's old four-sample condition list is historical; current source's owned phases
already use native samples. A future consolidated tool should reuse this machinery
and add only the missing boundaries, not revive the 10 GB ETW recorder.

No new run is requested by this audit. Next implementation work should be a bounded
measurement extension for the above gaps and an inspection of RELAX intermediate
formats/pass scheduling. A GPU or CPU redesign must then show a matched frame-time
win while preserving the accepted visual tests. A wholesale CPU-pipeline replacement
is not yet justified by the cost evidence.

## Reproduction and source anchors

Run `python research/performance-pivot/analyze_existing.py ARCHIVE_DIRECTORY OUTPUT_JSON`.
See `existing-comparisons.json` for raw-derived values, source hashes, settings,
coverage and CPU/GPU scope distinctions; `cpu-collector-inventory.json` for the
17 collector archives read in this audit.

Code: src/ui/performance_compare.h (phase application, grouping, settings restore,
coverage gates); src/performance_compare_state.h (current native-sample matrix);
src/rr_evaluation_entry.h (dimensions and evaluation timing boundary);
src/nrd_diffuse_runtime.h, src/nrd_diffuse_pipeline.h, src/nrd_diffuse_d3d12.h
(current bundle defaults and allocation/reuse); src/rt_diagnostics_mode.h and
src/probe.cpp (remaining ownership installs).

Earlier evidence: research/native-cpu/r404-user-result.md;
research/traversal/r411-trace-review.md; docs/Rendering-Work-Audit-R386.md;
docs/Performance-Compare-R392.txt; docs/RT-Preparation-R413.txt.
