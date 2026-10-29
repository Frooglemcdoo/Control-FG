# Runtime scheduler audit — 2026-10-29

## Binary identity

Uploaded `rl_rmdwin10_f.dll`: CodeView GUID
`5FF888E0-6D9F-4BE7-B002-78D9CE99F0CE`, age 1, image size `0x3E2000`.
Uploaded Control executable: CodeView GUID
`65682BEE-DDB8-4298-85F0-3FB323A977C6`, age 1, image size `0x13EA000`.
Each complete GUID-plus-age byte sequence occurs twice in each ETL,
consistent with image identification at start/end. The previously audited
renderer also matches. All following runtime addresses are build-specific RVAs.

## Verified scheduler contract

- `JobManager::createJob` (`0x807B0`) selects a per-thread pool using TLS
  and the requested priority. The pool uses 64-byte slots (`0x80D90`).
- Pool allocation can call `executeNextJob` when a slot is still busy
  (`0x80DC9`). Allocation itself is therefore a reentrancy boundary.
- `JobBase` construction (`0x33620`) initializes its vtable, counter at
  `+8`, and parent pointer at `+0x10`; native captures begin at `+0x18`.
  Do not place arbitrary-sized captures in a native job slot.
- The returned handle contains a job pointer and a generation value.
  Generation checks matter because pool slots are reused.
- `runJob` (`0x5DE10`) queues a job whose counter is one. Queueing changes
  state atomically, selects the priority queue, and wakes the worker system.
- Public `waitJob` (`0x5DE30`) checks handle flags, generation, and job
  completion. While incomplete, it calls `executeNextJob` at `0x5DE89`.
- `executeNextJob` (`0x5E110`) obtains a job from the manager and invokes
  its virtual execute slot. It is not restricted to the job being waited on.
- Execution updates counters/state, advances the generation, and can queue
  a dependent parent. This is not a plain submit-and-block thread pool.
- Scratch-pad state is thread-local (`0x1C1AF0`, accessed through TLS).
  Cross-thread scratch ownership cannot be inferred from ordinary allocation.

Consequences: never allocate or join native jobs while holding the material
manager's lock on the assumption these operations cannot run other work.
A candidate implementation must establish stable inputs, release locks,
dispatch, join, validate results, and publish on the owner thread. A pointer
copy alone does not establish input lifetime or prevent texture streaming.

## Existing trace attribution

The hottest game threads contain 29,060 (base) and 29,146 (modded) samples.
Using recovered stacks and exact return addresses:

| Path | Base samples | Modded samples |
| --- | ---: | ---: |
| Public waitJob, inclusive | 3,209 | 2,631 |
| Of those, renderer caller return 0x124053 | 2,647 | 2,110 |
| Public waitJob own instruction samples | 434 | 432 |
| Counter wait own instruction samples | 51 | 57 |

Renderer return address `0x124053` follows the join of the native
ray-tracing preparation job described in `worker-boundary-audit.md`.
It appears in 9.11% and 7.24% of the busy-thread sample totals. This confirms
the native parallel path was exercised during the recordings, not merely
present as dormant code.

These inclusive counts include scheduler lookup and potentially useful jobs
executed by the waiting thread. They are not elapsed job latency, pure idle
time, lost FPS, or CPU-only rendering work. Stack recovery is partial and
the two recordings are not an exact deterministic benchmark.

Own samples also land in getNextJob/executeNextJob. Therefore the previously
observed near-continuous scheduling of the rendering thread does not imply
it spent all of that time advancing a strictly serial rendering workload.

## Implementation decision

The next dependency investigation should prioritize the already-parallel
ray-tracing preparation task and its join, alongside the material-table loop.
Adding more material jobs without understanding the existing join could
increase scheduling overhead or contention.

Do not change worker count, affinity, spin/wait behavior, or dirty guards on
the basis of these samples. Before a parallel test build, identify the
preparation task's partitionable inputs/output ownership and prove that
texture/native-resource lifetimes span dispatch and join. No additional
broad capture is required for the static analysis. No runtime changes or
performance gains are claimed by this audit.
