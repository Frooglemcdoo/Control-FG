# R33 native-count regression fix

Evidence: Control-FG-Compact-Logs-20261019-144844-185e58cd.zip, latest process 24904. UI requested six, four, eight; RR_GI_SAMPLES_LIVE latched those selections. Native/effective remained 2 with full_rr=1. The old R30 process 27548 also transitions from native=1/effective=4 to native=2/effective=2. This is not a failed click or frame-latch issue, and the reason for the native count change is not established by this log.

Root cause is GISampleComparison's nativeCount==1 restriction. R33 allows explicit 2/4/6/8 requests to replace any native count in the audited tweakable range 1..16. Native selection, RR off/partial, and invalid native values remain untouched. This intentionally permits a lower explicit count too: a button is an exact user request, not a minimum. Allocation and shader count still receive the same native-local value.

Policy regression tests cover native=2 with all four selections as well as native=1/4/16, invalid counts, Native and RR-off preservation. Frame-latch and reset tests remain unchanged. R32 diagnostics are inherited, but no performance improvement is claimed. The submitted logs contain no R32 creation-audit records; do not use them as proof of R32 activation or the requested matched performance test.
