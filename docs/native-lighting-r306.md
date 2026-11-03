# R306 — GI resource lifetime integration

R305's four complete writer captures already validated 34,942 bindings, including
5,559 alpha-tested records. That result does not need repeating. R306 addresses
the diagnostic AS inventory retaining obsolete allocations until its 512 MiB
limit prevents new entries.

The earlier PT release hooks are adapted to the existing AS registry. The bridge
obtains the exact resource pointer and generation before native release,
forwards the native function once, and removes only that generation afterward.
Native release queues its own retained resource reference before clearing the
wrapper. Our build/copy submission leases remain independent and fence-gated.
The bridge drops only diagnostic references and does not change GPU work.

## Run

Install the full package beside Control_DX12.exe, preserving folders. Run
`ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd`, load the usual save, keep RT
indirect lighting enabled, and play/move around for approximately two minutes.
Exit normally and send the automatically packaged diagnostics ZIPs. No hotkey,
fixed camera or zoom is required. Whole-table and geometry captures remain
included so lifetime counters can be checked against the same session.

This run checks behavior R305 could not record: successful native retirement,
inventory growth/rejections after retirement, and continued GI/geometry capture.
Expected startup marker: `GI_AS_RETIRE_INSTALL ready=1`. `GI_AS_RETIRE_R306`
reports retired, untracked, stale and invalid wrapper counts. Untracked includes
normal non-AS buffers; it is not by itself an error.

## Limits

Replacement GI remains disabled. This is not a visual speckle fix. The same
512 MiB AS and 1 GiB geometry inventory limits remain; the budget counts indexed
resources, not all independent leases. Static validation covers 16 known release
calls in the supported DLL, not every possible resource lifetime. Missing
coverage remains explicit. Already acquired leases survive retirement.

No extra ray dispatch is authorized by native deferred ownership. Replacement
GI still needs independent traversal/geometry/material leases, visibility and
shading integration. Normal launch leaves these diagnostic hooks off. Writer
files retain their R305 schema/name for collector compatibility; the install
and retirement markers identify R306.

## Build validation

Remote code commit `11f91f68f9647935c16260737f9b1a1e490a84a2`, matching local
`2b11a6f`, tree `9cc8c8ac5aec33b02aeb1d8f6237b2faaea0845b`.
Windows foundation run `41674040317` / job `126049711402`: PASS, including
native forwarding, generation replacement, and blocked-queue lease retention.
Full build run `41674040266` / job `126049712029`: PASS.
Artifact `13249119782`: 74-file install ZIP, 4,226,337 bytes, SHA-256
`1b952f9f1ecb4ea076805ac71df8925c828ed201e6740325166c30b7d599d4d9`.
ZIP integrity, nested checksum, R306 DLL markers and launcher/instructions were
verified after download. Game-side retirement coverage is not yet validated.
