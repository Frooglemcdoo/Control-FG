# R247: initialize DirectStorage before texture demand

R245's latest run completed 9,627 accepted transfers without failure, but lane
zero initialization took 1.289 seconds. Its first 1,214 queued requests averaged
1.343 seconds waiting to begin submission. These are startup wall durations,
not proof of a later combat hitch.

R247 hands a retained native DIRECT queue from the first available Present to
the existing storage worker. The worker prepares both lanes, shares the compiled
pipeline, then resumes its normal request loop. Present does not create queues,
compile shaders, wait for readiness, or allocate texture payload buffers.
Initialization does not require a texture request. Temporary arenas remain lazy.

A lock-protected one-shot mailbox owns the queue until the worker takes it.
Invalid or absent queues leave the mailbox open for a later Present. If a texture
request arrives first, the established demand initialization still works. The
worker never initializes an already initialized lane twice. Initialization failure
disables new acceptance and already accepted jobs retry through the native path;
it does not report unfinished textures as complete. Driver initialization still
has a cost; this moves its opportunity earlier, rather than eliminating it.

Install the full package beside Control_DX12.exe. Launch with
Start-CPU-R247-NativeGPUStorage-4ms.cmd. Use the title menu normally, then load and
traverse once. Send compact logs and texture-arrival/stutter observations. No
R246 trace, Reference run, budget sweep, or standalone validator is required.

Expected new evidence:
- DIRECTSTORAGE_R247_PREWARM posted=1, with present and pending counts.
- DIRECTSTORAGE_R247_INIT reason=prewarm for both lanes (or first_request if demand
  won the race). Ready must be 1 for each successful initialization.
- DIRECTSTORAGE_R247_PREWARM finished=1 ready_lanes=2.
- Increasing accepted/completed counters, zero failed, and pending eventually 0.

Logs retain R245's optional stage summaries and their limitations: completion
means GPU work finished and native publication was dispatched, not that the
texture appeared on screen. Source archives, payload handling, scheduling,
resource states, fences, fallback, visual settings and 4 ms/16 scan partitions
are unchanged. Scan partitions do not directly set texture residency.

Validation adds concurrent one-shot mailbox admission, invalid queue rejection,
retained queue handoff, worker wake and no-repeat checks to the Windows test.
Existing 72-mip byte equality, arena reuse, consumer-queue independence, native
ownership, retry and scheduler checks remain. This does not newly certify forced
scene destruction, cancellation, or device recreation; those remain separate
lifecycle validation work. Live startup benefit still needs the user's run.
