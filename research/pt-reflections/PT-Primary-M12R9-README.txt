M12R9 - dead geometry retirement fix and exact handoff diagnostics

R8 recognizes the active primary-lighting shader but rejects every PT attempt
inside writer/submission readiness. The R8 log cannot identify the exact check.
R9 records descriptor rejection, replay, reset, pending writer/submission,
writer buffer/pipeline mismatch, and invalid/unowned binding rows separately.
These checks remain required; this is not a confirmed handoff/rendering fix.

Fixed a separate retention defect: history compaction kept the final geometry
version of released native allocation generations indefinitely. R9 discards
that metadata only after observed GPU completion, with no active historical
cuts or unresolved compaction cuts. Independent GPU copy leases still apply.
Tests cover pending GPU work, old consumers, reused addresses and actual
release of a snapshot owner. The active copy budget remains 512 MiB.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep the existing Streamline installation. Load the entrance save with native
RT enabled. Use F10 -> PT Reflections ON for 20 seconds. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R9.cmd, and upload its results ZIP.
No screenshot or repeated OFF/ON comparison is required for this diagnostic.
Stop early and collect logs if performance becomes unusable.
