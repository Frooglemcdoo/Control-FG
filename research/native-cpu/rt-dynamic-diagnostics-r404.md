# R404: identify the remaining dynamic-stage cost

The R403 rerun again reports no FPS benefit. Full first and rerun logs were
read; see r403-rerun-review.json for member hashes and installation checks.
Warm dynamic CPU averages are 0.890331 and 0.886902 ms/timed recording job.
These are different gameplay workloads and do not establish an A/B gain.
Existing earlier log inventory, native material and worker audits, and available
performance ZIPs were consulted. None splits the current R403 dynamic stage.
The older CPU-GPU capture v2 explicitly disables stack collection and is not
being requested as a substitute for the missing child timings.

## Verified native path

Renderer A67B0 is the skinned provider update callback. It binds current skinning
parameters (1CAC70), two provider buffers, mesh offsets/counts, current object
constants (1C7C30), chooses/applies shader state (1DD450/1DB750), and dispatches
via D3D 30690. 1C7C30 already has an atomic per-generation update guard; removing
that helper would lose required object-constant publication.

D3D 16A10 calls 16980. That setter copies the registered byte range to current
thread storage (+4250 plus registry offset), then increments the provider version
at TLS+50+4*index (skipping sentinel -1 on wrap). It does not compare contents.
This establishes redundant-dirtying as a candidate, not its runtime frequency
or cost. ShaderBuffer 417B0 resolves resource handles through NativeBuffer
fields +8/+10/+18 or fallbacks. Reusing an unchanged source pointer alone is not
a resource-generation proof. Do not suppress all equal-looking resource writes.
D3D setShaders 2F050 already compares the seven shader pointers before changing
its pipeline-dirty flag. Another wrapper around it cannot claim that saving.

## Runtime change

Same owned command job and same native calls. Split the old dynamic interval
into device retrieval, common bindings, transition into UAV, entire provider
loop, and transition back. Rename adapter `flush` to `bindCommon` to reflect
what renderer 10F160 does. No native patch or resource semantics changed.

One in 16 jobs is sampled, now ten QPC reads rather than six; no timer inside
provider/material loops. Timed update counts use the same sampled population.
Legacy coarse fields remain (dynamic equals the sum of the five raw intervals,
subject to integer microsecond rounding). All timing is CPU-inclusive; provider
loop includes child native work and possible preemption. A result identifying
that loop alone does not establish which child inside it dominates.

The exact native-machine-code oracle still compares 4096 cases and 68494
callback events. Alternate cases disable the phase callback; enabled cases
verify ordered phase boundaries and their actual preceding callbacks, including
empty/negative-instance cases. Context replacement, current material IDs, and
one native TLAS build per job remain covered. This is correctness validation,
not evidence of game performance improvement.

R404 is explicitly a diagnostic build. No FPS improvement claimed. Next runtime
optimization requires measured eligible cost plus an invalidation/lifetime
contract, not just proof that an owned wrapper executes.
