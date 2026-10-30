# R222: material and RT workload diagnosis

R221 established a native-append baseline: both append hooks absent, all append counters zero. Its steady 779-batch scene produced sampled means of 1.66ms material update, 3.80ms RT preparation, 1.07ms RT recording and 2.58ms preparation join. These overlap and must not be summed. R219/R220 gains remain unconfirmed; their hooks remain bypassed by default.

R222 is a diagnostic build, not a claimed performance optimization. R217 visual fixes, Map Guard OFF and angled reflections are unchanged.

## Run

Install over R221. Run `Start-CPU-R222.cmd`, hold the same problem-area camera/settings for 30–60 seconds without recording, exit, then run `Collect-ControlFG-Compact-Logs.cmd`. Send its ZIP and FPS range. The launcher explicitly enables CPU timing/workload snapshots and disables both append experiments using process-local environment variables.

## New measurements

Every 64 material updates, a wrapper at native callsite RVA 0xC83AF reads the manager's entry count (+0x80), sorted count (+0xD0), unique/output count (+0x100) and the existing native global rebuild counter (renderer+0x128D788). The call remains inside the native manager lock, after rebuild processing. The original debug-tail function runs first. No arrays or texture pointers are traversed. There are no allocations or file writes under the lock. The wrapper still adds a small per-update branch/call overhead, and sampled scalar reads are included in material timing.

Every 64 preparation jobs per worker, the existing timing wrapper samples the completed context's object count (+0x18), instance count (+0x78), geometry count (+0x88) and material-reference count (+0x98). Context comes from the job capture at +0x18. Snapshot collection occurs after the timing scope and before returning ownership to the native scheduler. Append TLS counters/publication remain bypassed.

`NATIVE_CPU_R222_INSTALL ready=1` confirms exact callsite/counter/layout instruction checks and installation. `NATIVE_CPU_R222_MATERIAL` reports cumulative samples/sums, latest sizes and latest native rebuild counter. Compare counter deltas, not its absolute value; it is a global 32-bit counter and may wrap or include other manager instances. Unique counts are the native uploaded count, including native sentinel behavior; do not interpret them as unique texture count. `NATIVE_CPU_R222_PREPARATION` reports per-worker samples/sums and last sizes. Sum sample deltas and value-sum deltas across workers for interval averages. Logs remain at five-second intervals and snapshots use bounded coherent atomic reads.

This distinguishes frequent whole-table rebuilding from preparation workload growth. It does not identify individual dirty-flag writers, prove resource lifetime for caching, or establish a safe parallel partition. The known texture callbacks can invalidate resources without changing material hashes, so neither dirty suppression nor persistent descriptor caching is introduced.

Set CONTROLFG_CPU_WORKLOAD=0 to omit the new tail hook and workload reads while retaining R221 timing. Normal launch enables workload diagnosis. Both append hooks still require separate explicit opt-in.
