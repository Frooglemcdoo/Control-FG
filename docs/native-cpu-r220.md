# R220: RT material-reference append fast path

R220 retains the R219 64-byte record optimization and adds a separate fast path for eight-byte material references in the same native RT preparation loop. Only the call at renderer RVA `0xA8DED` changes; other calls to the shared native append routine are untouched. Existing capacity uses the same count increment, complete pointer-value copy and return address. Growth and destination/header overlap delegate to the native routine. The copied pointer is never dereferenced by this optimization.

Record order, material/geometry indices, allocation ownership, LOD, native worker scheduling and GPU commands remain unchanged. R217 GI smoothing, Map Guard off and restored angled reflections remain intact.

## Compare R219 with R220

Install over R219, then run these launchers from the game folder, exiting Control between runs:

1. `Start-CPU-R219.cmd`: the accepted 64-byte fast path ON; new eight-byte fast path OFF.
2. `Start-CPU-R220.cmd`: both fast paths ON.

Use the same problem-area position, camera and graphics settings for 30–60 seconds per run, without screen recording. After both, exit and run `Collect-ControlFG-Compact-Logs.cmd`. Send its ZIP and FPS ranges. The scripts use process-local environment variables only; normal launch enables both paths by default. Do not use the older Native/Fast launchers for this comparison: they control the R219 optimization, not the new addition.

## Validation and interpretation

The original eight-byte native function was executed in an isolated x64 test through its Windows calling convention. Candidate and original produced identical records and return addresses for 4,096 unaligned appends. Ten alternating-order host microbenchmark pairs favored the specialized candidate; this does not establish an in-game gain. Tests also cover ownership flags, full capacity, unaligned storage, source/destination overlap and source/header aliasing. Count increments before the source read, matching the original. Windows tests check exact eight-byte copies against protected pages and verify native fallback forwarding.

The separate eight-byte helper leaves R219's 64-byte helper and its runtime hook implementation unchanged. Exact renderer image identity, original callee prefix/copy bytes and direct-call destination are checked before patching. Installation is startup-only, and failure retains native fallback.

`NATIVE_RT_POINTER_R220 installed=1 enabled=1` confirms the new path. `NATIVE_RT_POINTER_R220_STATUS` reports fast/native pointer append counts for completed preparation job callbacks. Existing R219 counts report the 64-byte path. Both timing and counter summaries remain low-rate; the inner loops use thread-local counters and publish at job completion. A serial preparation fallback outside those callbacks is not included in published counts.

R219 user results were 65–67 FPS Native versus 70–73 Fast. The logs verified both requested modes and roughly 139 million fast record appends. Their scene workloads differed, so the approximately 8% midpoint FPS difference is an encouraging observation, not a controlled attribution of the entire gain to that operation. R220's incremental gain is unconfirmed and should be judged separately.
