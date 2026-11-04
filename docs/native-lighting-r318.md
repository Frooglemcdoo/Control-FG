# R318 — Owned GI integration

Install the package over the existing mod, preserving folders. With Control
closed, run `ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd`. Keep RT indirect
lighting and RR enabled and select GI RAW BYPASS. Leave other settings unchanged.
Use the F10 GI button to compare NATIVE and OWNED while standing still and moving
around the lit plant/hair scene. Allow RR history to settle after switching.
Exit normally; the launcher packages diagnostics without a capture hotkey.

`GI: OWNED` means the CPU admitted a replacement pass for submission. It is not
a GPU readback or a claim that the image is correct. `GI: OWNED WAIT` means the
requested path has not been admitted, or its most recent admission failed;
native output remains. Persistent WAIT needs the automatically collected logs,
not another capture. NATIVE disables new owned recordings; already submitted
frames finish normally. The existing sampler stays enabled in both comparisons.

This build connects five owned stages to the real GI6 boundary: finite-range
proposal generation, private DXR tracing with native alpha/material handling,
secondary-hit lighting, initial reservoir estimation and native-format output
resolve. Admission uses the current native bindings and ordered TLAS/BLAS scene,
with build-time copies of index/position geometry. Missing or unverified scene
data leaves native output intact. The current path requires one native GI pass;
other pass counts remain native and are reported in diagnostics.

It is an initial owned producer/estimator integration, not the completed GI
modernization. Native GI6 still runs first for fallback, and no spatial or
temporal sample reuse is active. Full-frame processing uses bounded scratch
tiles; the scratch reservoirs are not persistent history. Additional GPU work
and geometry storage are expected, so this is not a performance release.

Geometry copies and scratch each have a 512 MiB active limit; pooled idle
geometry is limited to 64 MiB. Owners are retained through queue completion.
The submission prefix arms each pass once and the GPU clears it after resolve,
so a completed-list replay cannot rerun that owned pass. The live path adds no
CPU fence wait and no image readback. Logs distinguish recording, admission,
rejection reasons, completed queue checkpoints and incremental compactions.
Scene metadata is compacted between admissions using polled completion stamps;
it does not require every queue to become idle.

The existing RTX 5090 standalone producer result is reused. This package needs
an in-game integration check, not a repeat of that standalone test. Report the
toggle status, speckles, missing lighting, color changes, flicker or frame-time
regressions and send the generated diagnostics ZIP. Launching normally leaves
the experimental owned path off. DirectStorage settings are unchanged.

R311 fixes descriptor and GI6 binding observer installation at native device creation. R310 attempted this during module setup, before the device existed, and never retried. The supplied run recorded missing_native_bind, zero admissions, and active geometry preservation. The new GI_OWNED_DEVICE log reports descriptor and binding installation separately. This corrects the observed initialization gap; live scene admission still requires game validation.

R312 adds consolidated binding diagnostics after R311 verified that hooks install but no binding snapshot reaches dispatch. GI_OWNED_BIND_RESULT records callback counts, command-list/writer rejection, provider rejection and sparse descriptor-index health. This build does not claim to fix the remaining activation failure. The ordinary launcher run and automatic logs supply the missing evidence; no GPU capture or repeat standalone test is required.

R313 replaces the same-command-list writer lookup with a single-use handoff
bound to the exact native GI invocation and engine frame. Native raytrace may
rotate command lists before GI6; the receiving list retains its own generation.
The queue observer requires the exact producer recording to occur before the
consumer on the same queue. It checks this at the consumer event, before later
lists in the batch, and rejects abandoned/reset producers and replays. Other
queues remain unsupported rather than inferred to be ordered. No graph cut is
retained across metadata compaction and no extra CPU/GPU wait is introduced.

The R312 capture already supplies the failure evidence: all GI6 binds reached
29 descriptor-copy callbacks and one CBV callback, the AS descriptor index was
healthy, but each bind rejected with writer_missing. The existing writer exports
prove CPU snapshots can be taken; they do not prove owner publication. R313
therefore reports publication failures and both producer/consumer generations
alongside the handoff fix. Activation and visual quality still need validation
in Control. No repeat standalone producer test or RR capture is needed.

R314 avoids expensive owned scratch/SBT setup and tiled command recording when
an early check of the submitted scene already fails. This check never authorizes
a pass: submission still repeats the exact ordered-scene validation. If this
recording contains new AS writes or its producer has not yet been submitted,
validation remains at submission so recording order cannot substitute for GPU
order. Rejected preflights log the scene stage, graph status/reason, offending
allocation and instance, and total unavailable instance references. Build-time
rejections additionally distinguish instance snapshots, geometry descriptors,
resource ownership/state and frozen-copy failures.

The owned launcher now disables the independent R294 diagnostic inventory with
CONTROLTOH_GI_LEGACY_AUDIT=0. Live creation/storage proofs and scene callbacks stay
active. Other diagnostic launchers retain their previous default. R313 retained
about 1 GiB in that older inventory and observed about 1.87 million barriers, in
addition to the live scene renderer. Removing it avoids duplicate bookkeeping;
it does not disable live geometry preservation or promise a measured FPS gain.

R313 proved writer publication, native inputs and cross-list binding snapshots
work, but all reported admissions failed at ordered_scene. Its sampled binding
to recording intervals were approximately 24–27 ms; these intervals are not
isolated setup timings. R314 addresses wasted setup and diagnostic overhead.
The precise missing scene dependency was not recorded by R313, so this build
must not be described as a verified activation fix. Use the ordinary launcher
and automatic exit logs; no RR capture or standalone producer test is needed.

R315 connects the live scene catalog to the existing fence-completion observer.
R314 rejected every reported scene at tlas_order / scene_dispatch_wait_pending
while the game continued rendering; the live adapter was never feeding native
fence completion into the graph. Initial or otherwise unobserved completed
signals could therefore leave the CPU model pending indefinitely. The new path
polls only fences with an unresolved, unobserved signal range. It adds no CPU
wait, does not resolve future values or device-removal sentinels, and preserves
observed GPU signal edges. It does not invent ordering between unrelated queues.

Queue-wait preflight now runs before the new-AS-writes exception, preventing
expensive owned pass allocation/recording while that known blocker persists.
The previous content preflight deferred on new AS writes and did not catch it.
Build-time geometry preservation still runs; this change is not a guarantee
that the reported 20–30 FPS will return to the normal 80–90 FPS.

Existing ordering tests are reused and catalog-level tests cover completion
forwarding, insufficient completion, device loss, future waits, updates after
resolution, compaction and unrelated queues. In-game activation and performance
remain unverified. Use the same launcher and automatic exit ZIP, with no RR
capture or repeat producer test. GI_OWNED_FENCE_COMPLETION records resolved
waits; queue-wait preflight reports the fence and exact requested value if blocked.

R316 adds a 250 ms retry interval after a failed owned admission. This skips
repeated pass preparation while retaining native output and live geometry
tracking; success restores normal per-frame attempts. It is an overhead guard,
not a claim that owned GI now activates or that all FPS loss is corrected.

R315 still rejected all reported admissions at scene_dispatch_wait_pending.
It logged no resolved fence waits or early queue-wait rejections. Existing PT
tests already demonstrate pending native submissions whose producer signal is
issued later, including cases where that signal changes the valid AS version.
Do not promote recording previews to admission or bypass ordering checks.

The automatic logs now include the exact pending fence, requested and observed
values, actual completion, queue lane and cut at rejection; the first two
failures dump a bounded 256-event queue history. Preflight-path logs distinguish
unknown queues and producer ordering. CPU timings aggregate geometry
preservation, writer snapshots, native binding snapshots, pass preparation,
queue tracking and admission. Timings are inclusive and must not be summed. The binding stage includes the
original native bind call, so its total is not purely injected overhead.
The retry changes execution frequency, so compare calls and totals as well as
maximum costs. Logs identify R316 explicitly. No image readback or GPU wait is
added. This missing rejection-time history was not present in earlier captures.

R317 hooks the exact game queue interfaces at Control's existing command-queue
constructor boundary, before their first game submission. R316's unknown
preflight queue address exactly matches SL_QUEUE_ROUTE_END's direct proxy,
not its underlying native queue. The recorded internal queue waits on fence
52 values 4, 12, 16, etc. precede later releases from queue 54.

The existing nested-call guard now observes the outer game submission once
and suppresses synchronous native forwarding. The owned catalog accepts only
registered game queues, excluding middleware/transport queue fence traffic.
Any scene events on an unregistered queue invalidate admission and retain
referenced resources; they are not silently ignored. Actual game cross-queue
waits/signals and strict geometry checks are preserved. Prefix commands and
completion fences use the same game queue interface.

This changes the observation boundary, not the GPU's queue ordering or fence
values. It avoids prematurely committing controls after submission, guessing
a late signal, or promoting recording previews to valid scenes. The R316
sequence is reproduced in the existing scene-order test, with a synthetic AS
version; it is not a full replay of unavailable scene payloads.

The captured CPU costs prioritize further geometry and tracking work: across
59 active windows, median cost per writer invocation was 5.715 ms geometry,
2.788 ms queue tracking and 0.854 ms throttled preparation; preparation itself
was 9.366 ms per attempt. These inclusive CPU metrics are not a complete frame
profile. R317 removes irrelevant queue tracking; geometry copying remains
unchanged until reuse can be proven safe. Keep the 250 ms failure throttle.

In-game activation and FPS are unverified. Use the same launcher, GI RAW BYPASS,
RR and RT indirect lighting; exit normally to collect logs. No RR capture or
repeat standalone test is required. GI_OWNED_GAME_QUEUE revision=R317 confirms
registration of the game interfaces; GI_OWNED_ADMITTED confirms CPU admission.

R318 corrects an overly early wait rejection. R317 registered all three game
queues and linked the writer to queue 1, but preflight rejected every attempt
while recording: fence 37 values 3, 7, 11, etc., with the observed signal one
frame behind. No replacement was recorded or admitted. This does not prove
that the fence remains unresolved at the later Execute boundary.

An unresolved recording-time wait now defers to strict submission admission.
The pass remains disarmed until its exact scene and resources are validated;
no preview is promoted, no late controls are edited, and no fence is invented.
The existing 250 ms failure throttle remains for rejected submission attempts.
The catalog regression covers a pending recording cut, a producer update and
later sufficient signal, and rejection of an unresolved next-frame wait.

R317 capture integrity: all 15 files verified by size/SHA-256. Zero admissions.
Across 41 active windows with 20–40 writers, median inclusive CPU milliseconds
per writer were geometry preservation 6.744, writer snapshot 0.465, preparation
0.040, and queue tracking 1.835. The preceding R316 run had 5.715, 0.455, 0.854,
and 2.788 respectively across 59 windows. These are different runs, not a
controlled FPS comparison. Geometry preservation remains a substantial cost;
it is unchanged in R318. Activation and actual performance remain unverified.
