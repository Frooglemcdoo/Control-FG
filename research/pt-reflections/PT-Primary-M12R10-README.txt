M12R10 - isolate untracked scene writes and diagnose ownership precisely

R9 stopped scene capture before PT was enabled. Its combined error did not
separate destination ownership from command-list recording failures.

R10 rejects untracked AS destinations locally. Every live alias at that
address receives an ordered invalidation, so older geometry cannot be reused.
Without a live owner, the allocation cannot pass dispatch lease validation.
Future owners receive fresh tokens and require a new observed build.
Known destinations still require valid list generations and queue ordering.
Tests cover untracked build/copy, ambiguous aliases, undersized destinations,
and recovery after a subsequent valid build.

Logs now separate list/generation failures and geometry owner failures, and
include registry budget/rejection counts. R9 handoff diagnostics remain.
This fixes the global failure policy; PT rendering is not yet confirmed.
Memory limits and all native-fallback/dispatch validation remain enabled.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep the existing Streamline installation. Load the entrance save with native
RT enabled. Use F10 -> PT Reflections ON for 20 seconds. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R10.cmd, and upload its results ZIP.
No screenshot is needed. Stop early and collect logs if performance is unusable.
