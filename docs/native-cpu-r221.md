# R221: native append bypass baseline

R219 gains did not reproduce and the R220 comparison was slower overall. Neither optimization is treated as a confirmed gain. R221 defaults to installing neither append hook: the original three E8 callsites, native destinations and inner loops remain untouched. The preparation timing wrapper skips append TLS snapshots and atomic counter publication when neither hook is installed. Existing low-rate CPU stage timing remains enabled; this is a native-append baseline, not an entirely uninstrumented game.

R217 visual fixes, GI smoothing, Map Guard OFF and restored angled reflections remain unchanged.

## Run

Install over R220. Exit the game and launch `Start-CPU-R221-Native.cmd`. Stand in the problem-area view for 30–60 seconds without video recording. Exit, run `Collect-ControlFG-Compact-Logs.cmd`, and send the ZIP with the FPS range. Normal launch also defaults to bypass unless an explicit opt-in environment variable is inherited.

Optional comparisons use `Start-CPU-R221-R219.cmd` or `Start-CPU-R221-R220.cmd`, exiting between runs and matching camera/settings. These reproduce the old instrumented R219-only and both-fast-path configurations, including the disabled pointer forwarding hook in R219 mode. They are diagnostic candidates, not accepted optimizations. All three scripts explicitly enable stage timing and set process-local variables.

`CONTROLFG_CPU_APPEND_HOOKS=1` is the only opt-in; unset, zero, malformed or oversized values bypass installation. Old launchers do not set this variable and should not be used for comparisons. Bypass logs `NATIVE_CPU_R221 append_hooks=0 native_calls_untouched=1 counters=0`; both append STATUS records should show installed=0 and zero counts. CPU timing may independently be disabled with CONTROLFG_NATIVE_CPU_TIMING=0.

## Evidence

Latest logs: PID1904 used both fast paths; PID63272 used R219 only. Busy-scene RR samples (replay_batches >=650, reset=0) had median cadence 16.2256ms (7 samples) versus 15.3887ms (11 samples). Workloads differed (682–811 versus 675–861 batches), so this is not a controlled regression percentage. Post-loading sampled join means were approximately 2.61ms in both. No further gain is claimed.

Windows runtime tests verify default/malformed opt-in bypass without accessing an invalid renderer address, explicit opt-in parsing, forwarding/LastError, timing and zero counter publication in bypass. Existing patch, append, protected-page and visual shader tests remain in CI.
