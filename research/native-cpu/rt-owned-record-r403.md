# R403 owned AS command recording

## Scope and evidence
R402 larger-record optimization covers only 0.1178% of inferred objects in the
existing R393 upload capture. Do not request another capture for that alone.
Earlier phase measurements (R227) show prebuild sizing 0.011580 ms and buffer
management/upload 0.062787 ms, versus 3.223527 ms scene selection/assembly.
These are an older instrumented interval, not current comparable FPS evidence.
Earlier native outer recording averaged 1.013 ms; it lacks internal attribution.

The native preparation join at 12404D immediately precedes stream buffer
unmaps/clears at 124092–124100. Moving that join past the unmaps is not safe.
This change retains the scheduler, joins and buffer lifetimes.

## Replaced stage
The verified job wrapper A8F70 adds 0x18 to RCX, then tail-jumps at A8F74 to
A89C0. Replace only that five-byte tail via a near relay to owned C++ Run.
Verify the entire original 550-byte body, wrapper and tail before installing;
verify all eleven imported D3D entry addresses against the loaded module exports.
Keep the native A89C0 body available but do not invoke it through this routed job.

Owned sequence: choose native queue, initialize inherited thread state, preserve
initial signed instance count, run dynamic update callbacks within native UAV
transitions, run primary/secondary BLAS callbacks, copy the current postbuild buffer
to the parity-selected destination, run finalization callbacks, patch current
uint16 shader material IDs into geometry+36, issue native UAV barrier and exactly
one native TLAS build (flags=4, update=false), finish thread and publish cleanup.
Native callbacks, globals, buffer references and their read points remain live.
No additional resource owner, build, fence, wait, worker, or queue is introduced.

## Validation
4096 original-machine-code comparisons, 68,494 matching callback events, including
the actual replacement tail/relay bytes. Covers dynamic context replacement during
callbacks, current material IDs, signed zero/negative instance count branches,
queue choice, parity, barrier/copy order and six-argument TLAS ABI. Exact geometry
bytes and cleanup result also match. Windows CI runs this with /W4 /WX and the
existing preparation, selection, assembly, ownership and retirement tests.
The original body fixture comes from the verified renderer, not a hand-written
reference implementation. Tests replace only imported calls, globals and the
native flush callee with recording mocks; branches/body stay unchanged.

## Timing and interpretation
One job in sixteen uses six QPC reads at five coarse phase boundaries. No timers
inside provider/material loops. Reentrant timing state is scoped per invocation.
Log cumulative phase microseconds with timed_jobs. These are CPU inclusive times
(including native callback work), not GPU durations. Full native AS management is
NOT yet replaced. R403 is a parity/ownership milestone plus targeted attribution,
not evidence of FPS improvement. A game run is needed to validate this newly
routed job and measure its internal cost; older outer-job totals cannot do that.
