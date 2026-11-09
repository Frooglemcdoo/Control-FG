# R290 — GI geometry coverage test

Extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd, keep RT indirect lighting on,
move around for one minute, exit normally, and send the automatic diagnostic ZIP.
No hotkeys or stationary camera required. Normal launch leaves the observer off.

R289 remained OBSERVING for 5006 AS submissions: 1416967 successful range lookups
and 3765 failed lookups. These are repeated lookups, not unique scene coverage.
The owner inventory reached 2048 slots below its byte budget.

R290 increases metadata slots to 8192 while retaining the same 512 MiB logical
buffer-extent budget. Known-object observations use a pointer index; address
resolution uses sorted ranges and prefix maxima, preserving rejection of
ambiguous overlapping ranges. Geometry protection, candidate priority and atomic
admission refusal remain. Resource references still do not prove stable GPU bytes,
placed-heap ownership, aliasing, residency or cross-queue dependencies.

GI_METADATA_R290 adds missing/ambiguous/invalid counters and latest submitted vs
completed fence values. metadata_fence_ready only describes descriptor submission
completion. GPU geometry bytes and live triangle reconstruction remain unverified.
GI_OWNER_R290_SAMPLE logs a bounded rolling sample of up to 128 missing ranges:
kind 0=vertex,1=index,2=transform,3=instances; reason 0=missing,2=ambiguous,3=invalid.
Sample hit counts apply only while a sample is retained, not session-wide unique
counts. Samples update automatically alongside the five-second status reports.

Journals retain their 65536-event limit. Replacement GI remains disabled;
no speckle correction or production performance gain is claimed for this build.
