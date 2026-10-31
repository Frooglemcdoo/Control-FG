# R245 texture-arrival latency measurements

The user reports better frame pacing and no stuttering across the R244 budget
runs. Preserve that working streaming implementation. The 4 ms and 12 ms logs
confirmed their budgets and showed no queue-full fallbacks or failed transfers,
while visible texture-arrival speed appeared unchanged.

Install over R244 and run Start-CPU-R245-NativeGPUStorage-4ms.cmd. Use one normal
traversal, note where texture detail appears late, and collect compact logs.
16 partitions remain pinned. 8/10/12 ms launchers remain available, but another
budget sweep is not required. Every included launcher enables
CONTROLFG_STORAGE_LATENCY=1; setting it to 0 disables the new measurements.

## What is measured

Five-second cumulative DIRECTSTORAGE_R245_LATENCY rows contain sample count,
mean/max microseconds and counts at or above 16 ms and 100 ms:

- native_allocation: original renderer allocation call, for accepted new jobs.
- request_admission: request-hook entry through metadata/range checks and admission.
- queue_to_submit_begin: admission until the worker begins batch submission;
  includes first-use lane initialization when applicable.
- storage_completion_observed_batch: DirectStorage Submit start to the worker
  observing storage completion. Includes I/O, staging and worker scheduling.
- gpu_record_submit_cpu_batch: CPU command recording, execution submission and
  completion event registration.
- gpu_completion_observed_batch: ExecuteCommandLists start to worker observation
  of its completed GPU fence. Includes GPU queue wait and worker wake delay.
- request_to_handoff_begin: accepted request-hook entry to successful GPU
  completion and start of native handoff; also grouped by requested mip level.
- native_handoff_call: elapsed native finishLoadLevel call (deferred publication
  dispatch). This is not the delay until the renderer consumes that dispatch.

GPU-observed timings are CPU wall times, not GPU timestamp-query execution
measurements. The record/submit measurement overlaps the GPU-observed interval;
these metrics must not be added together as disjoint stages. Per-batch samples
must not be compared as per-request counts. Failure/retry jobs do not contribute
to successful completion/end-to-end metrics. Queue/admission metrics can include
jobs that later fail. Counters are concurrent cumulative summaries, not an
atomic transaction snapshot. No payload bytes or filenames are logged/read back.

This build adds no renderer publication hook, payload checksum or screen-visible
mip observer. It does not establish when the engine first wanted a higher mip,
or when the final draw began using it. If measured request-to-handoff latency is
short during late detail arrival, the next investigation is native request
selection or deferred publication. If it is long, these stages narrow the wait.

## Preservation and validation

GPU transfer, embedded shader, shared pipeline, reusable arenas, 4,096-descriptor
queue and native lifetime/publication remain unchanged. The default saved menu
budget is unchanged. Added clock reads/counters have nonzero overhead; summaries
are bounded and there is no per-request logging or new GPU work.

Windows tests verify observed storage/GPU completion markers, disabled-mode
behavior, timing thresholds, and all existing 72-mip content, buffer reuse,
scheduling, native ownership and rendering checks. Live confirmation still
requires the user's run; no texture-speed improvement is claimed here.
