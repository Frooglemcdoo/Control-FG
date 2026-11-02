# R291 — GI index-buffer coverage test

Extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd, keep RT indirect lighting on,
move around for one minute, exit normally, and send the automatic diagnostic ZIP.
No hotkeys or stationary camera required. Normal launch leaves the observer off.

R290 remained OBSERVING through 3419 AS submissions, with completed metadata
fences at the final report. It recorded 773443 successful range lookups and 48776
missing lookups, with no ambiguous or invalid ranges. All 128 distinct keys in
the retained miss sample were index ranges. Samples are bounded and repeated:
these counts are not unique scene coverage or a controlled comparison with R289.
The inventory remained below 8192 slots but repeatedly reached its 512 MiB budget.

R291 ranks default-heap buffers observed in INDEX_BUFFER-capable states above
generic shader-readable candidates, including promotion of already-known objects.
State hints do not prove buffer contents; GENERIC_READ includes INDEX_BUFFER.
Previously resolved geometry remains protected. Slot and logical buffer-extent
budgets remain 8192 and 512 MiB. No state transitions or GPU copies are added.

GI_METADATA_R291 adds session miss counts for vertex/index/transform/instance
ranges. A bounded CPU-only history retains latest admission/eviction/rejection/
exclusion events for up to 16384 buffer base addresses. GI_OWNER_R291_SAMPLE
includes the latest historical containing range at report time, not necessarily
at the original miss. historical_event: 0=unknown,1=admitted,2=evicted,3=rejected,
4=excluded. historical_rank: 0=other,1=default,2=shader candidate,3=index-capable.
Unknown history can mean unobserved or overwritten; historical address matches
cannot prove current resource identity, because GPU addresses may be reused.
History overwrite/allocation-drop counters make truncation visible. This history
holds no COM references and never resolves a missing resource or enables reuse.

Journals retain their 65536-event limit. metadata_fence_ready proves only observed
submission completion, not stable GPU geometry bytes, states, heap ownership,
residency, aliasing, or cross-queue byte dependencies. Replacement GI stays off.
This build tests geometry coverage; it does not claim to fix the speckles yet.

Validation: focused Windows run 41492681813 passed, including actual runtime
priority/refusal/history assertions, descriptor/index/ledger checks, and both
GPU adapter executions. Full Windows run 41492681970 passed all 53 steps.
Build commit: ef859dc8632fda43eb0659e816318d88e8c1fb5a. Initial build attempts
caught a reporting-call argument mismatch; the corrected source was rebuilt
and both final job logs were checked for assertion, HRESULT, and compiler errors.
