# R129: isolate the R128 GPU capture from gameplay

R128 was reported to crash immediately after loading a save, before the user
could request capture or save a report. No crash dump is available; the precise
faulting instruction and driver failure are not established.

R128 installed five device descriptor hooks, allocated GPU capture resources,
and compiled independent diagnostic DXR/compute pipelines during normal startup.
Those operations ran even when no capture was requested. R129 disables that
entire diagnostic GPU path with one build-time policy. The startup pipeline
call site is excluded, Install returns before allocation or vtable mutation,
and Request, BeginScope, Record and PreparePipeline reject activation. BOTH
cannot turn it back on. There is no INI override.

This is crash containment, not a claim that the capture implementation is repaired.
The diagnostic source and offline tests remain available for further investigation.
CPU table auditing remains available, but no actual GPU-hit result is expected.
The existing lighting modes, centered shadow assets, GI filter, material guides,
glass and fixed 60% clamp are retained.

## Test

Close Control and copy the package beside Control_DX12.exe, preserving NVIDIA
runtimes and ControlFGStreamline. Load the same save that crashed in R128.
Check normal gameplay first, then use the existing lighting modes if loading
succeeds. No 70-second capture or forced crash reproduction is requested.

The session log is written automatically under
`%LOCALAPPDATA%\Control-FG\Diagnostics` (temporary-folder fallback).
SAVE LOG is optional; it flushes the current report. If loading still crashes,
the latest `Lighting-R129-*.log` can be retrieved after the game has closed.
A crash may truncate the final queued messages.

## Validation

The Windows WARP test invokes the real release installation twice, before and
after diagnostic installation, and verifies that descriptor vtable entries are
unchanged and no capture GPU resources or original-hook pointers are populated.
Even a forced ready flag cannot arm capture through Request or BeginScope.
PreparePipeline rejects before touching a device, and Record is inert.
Existing translator, table-submission and log-export tests still run offline.
CI cannot establish that the reported live RTX 5090 crash is resolved.
