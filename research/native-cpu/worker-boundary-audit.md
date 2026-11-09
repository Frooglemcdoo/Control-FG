# Worker boundary audit — 2026-10-29

This is a static dependency audit, not a runtime optimization or a claim of
measured parallel speedup. It uses the renderer build identified in
`material-id-audit.md` and the available D3D DLL. No new recording is required
to continue this analysis.

## Existing native jobs

The rendering stage at renderer RVA `0x123D60` already contains native job
creation, dispatch, and joining. Imports identify `r::JobManager::createJob`,
`r::JobBase` construction, and `r::experimental::JobSystem::runJob/waitJob`.

### Ray-tracing preparation

- A conditional path creates a priority-1 job at `0x123F19`, constructs its
  base at `0x123F26`, assigns vtable `0x6281E8`, and copies a small capture
  into the object beginning at offset `+0x18`.
- Dispatch occurs at `0x123F62`. The serial alternative at `0x123F6E` calls
  the same captured work directly.
- Vtable execute entry `0xA8F80` forwards to `0xA81B0` with `this + 0x18`.
- The body includes view/convex-volume work, buffer readback, acceleration
  structure size estimation, and ray-tracing buffer allocation/upload.
- The stage joins the job at `0x12404D` before later dependent work.

This establishes an existing parallel path, not whether its runtime switch
was enabled in the supplied captures. Do not flip the switch without auditing
its intended configuration and dependencies.

### Ray-tracing command recording

- After `UpdateMaterialIDManager`, the stage obtains its thread-local device
  state and fills an inherited-device-state structure (`0x12499A`,
  `0x1249AA`).
- It creates another priority-1 job at `0x1249C4`, assigns vtable `0x6281D0`,
  captures a pointer at object offset `+0x18`, and dispatches at `0x124A01`.
- Execute entry `0xA8F70` forwards to `0xA89C0`.
- The body calls `ThreadingCore::initializeThread`, retrieves per-thread
  device state, records buffer transitions and acceleration-structure work,
  and calls `ThreadingCore::finishThread`.
- Nearby continuation code includes job joining, queue fences, deferred
  cleanup, and submission. These operations need to be traced as a complete
  producer/consumer chain before inserting another job.

The vtables' RTTI names are native `r::JobCallable<lambda>` instances. Their
small capture layout does not establish the scheduler's allocation, handle
ownership, cancellation, or wait semantics.

## Material-table partition assessment

| Work | Observed dependency | Proposed treatment |
| --- | --- | --- |
| Drain pending material updates | Manager locks and shared queue | Keep on owner thread initially |
| Read native material/texture structures | Mutable pointers; texture-change callbacks | Not approved for worker access without lifetime proof |
| Process copied material bytes | Potentially independent if fully copied | Candidate CPU-only worker input |
| Sort/deduplicate and assign IDs | Determines shared IDs and output ordering | Keep final publication serialized |
| Resolve 35 texture slots | Streaming-sensitive native pointers | Main unresolved safety/performance boundary |
| Write descriptor array/version | Shared descriptor manager state | Keep serialized initially |
| Stage/upload data and submit commands | Native thread-local device state and scratch allocation | Preserve native threading protocol |

The current rebuild owns a manager lock. Moving its existing body onto
several workers would expose shared writes and scratch/device state.
Holding that lock while joining workers is not automatically safe: a worker
or its callees may need the same lock. Conversely, dropping the lock after
copying pointer values does not keep their referents alive.

The exported native-texture getter only returns a stored pointer. No
retain operation is implied by calling it. The checked exports do not yet
establish a lifetime protocol suitable for a worker snapshot. Copying fully
resolved handles on the render thread would preserve more safety, but would
also leave the measured hot texture-resolution loop on that thread.

## Runtime binary received

The requested runtime DLL and game executable were supplied on 2026-10-29.
Both CodeView GUID/age records match the existing base and modded traces.
See `runtime-scheduler-audit.md` for the verified scheduler behavior and
sample attribution. The missing-runtime prerequisite below is resolved.

## Original required evidence (resolved)

`rl_rmdwin10_f.dll` from the same Control installation is needed to inspect
the implementation behind the imported scheduler functions, JobHandle
ownership, work performed during waits, and worker-local scratch handling.
The workspace contains the renderer and D3D DLLs, but not this runtime DLL;
the available file search did not resolve a matching upload.

With that binary, verify the scheduler contract before choosing native jobs
versus a dedicated CPU-only mechanism. Resource lifetime and invalidation
still require a separate check in the renderer/D3D code. Do not infer that
obtaining the runtime DLL alone proves the material loop parallel-safe.

No runtime patch or test build was produced by this audit. R217 remains the
accepted runtime baseline.
