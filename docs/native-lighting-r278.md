# R278: GI runtime accounting comparison

This diagnostic runs the no-reuse surface/fallback calculation inside the actual
hash-pinned native GI6 dispatch. The calculation uses private registers and the
existing diagnostic UAV; all native lighting instructions and output writes stay
intact. This validates integration before any replacement or sample reuse.
It is not a speckle correction.

## Run

1. Install the package beside Control_DX12.exe, preserving its folders.
2. With the game closed, run `ControlTOH/Diagnostics/Start-GI-Ray-Trace.cmd`.
3. Load the original plant/hair area with the lamp intact. Keep RR and RT indirect
   ON, Native Blend selected, FG OFF, GI RAW BYPASS and REF CLAMP: BYPASS.
4. Press F7 once. Wait for the visible five-second ARMED countdown, hold the view
   through SAVED, then exit normally. Mouse zoom may remain held.
5. Send the launcher-generated ZIPs. Do not use CAPTURE RR or PACKAGE RR.

R277 remains valid for the original ray analysis. This test measures a newly
executing calculation inside the game; it does not repeat that earlier analysis.

## Recorded comparison

Each ray contributes to either a private surface sum or a private miss-fallback
sum. Both are normalized by the native depth scale and total ray count, including
misses and black hits. The summary adds surface RGB at byte 224, marker 0x278 at
236, and fallback RGB at 240. Ray records and all earlier summary fields retain
their R277 layout. The 256-byte stride and allocation cap do not change.

The offline analyzer compares both terms with the complete ray records, and their
sum with native final RGB. It rejects nonfinite results, mixed versions and
differences exceeding 1e-6 plus 1e-5 relative tolerance. Older R277 traces remain
readable but are not reported as runtime-accounting verified.

The GPU fixture executes the exact injected arithmetic and store blocks. Removing
the injected instructions recovers the native shader byte-for-byte. Resource
binding, asynchronous completion and root restoration use the existing trace
path. The extra calculation runs during one armed diagnostic dispatch only.

## Limits

This is sampled, same-dispatch validation, not temporal or spatial reuse. It does
not correct the producer PDF, perform new visibility rays, identify scene objects
from material IDs, or establish improved image quality/performance. The game
continues to receive its native output. Normal launch leaves this probe disabled.
