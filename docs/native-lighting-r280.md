# R280: GI instance submission metadata

R279 exhausted its build logging allowance before TLAS builds. R280 retains up
to eight TLAS build descriptors in each command-list generation independently
of that allowance. Successful list reset clears the records and changes their
generation. Before native queue submission, up to 32 lists with TLAS metadata
are reported. F7 rearms this reporting window when its countdown completes.
Overflow and report limits are explicit. No geometry or lighting is changed.

## Run

1. Install the package beside Control_DX12.exe, preserving folders.
2. With the game closed, run ControlTOH/Diagnostics/Start-GI-Geometry-Probe.cmd.
3. Load the plant/hair area with the lamp intact, RT indirect and RR ON,
   Native Blend, FG OFF, GI RAW BYPASS and REF CLAMP: BYPASS.
4. Press F7 once. Wait for ARMED then SAVED, then stay in the scene another
   two seconds before exiting normally. Mouse zoom may remain held.
5. Send the generated ZIPs. Do not use CAPTURE RR or PACKAGE RR.

The reused ray trace format still says R278; that is expected. New metadata
uses GI_INSTANCE_R280 markers in the collected lighting log.

## Limits

This is a native submission observer, not a completed GPU-memory snapshot.
It reports queue/list identity, generation, array position, AS addresses,
instance-buffer address/count, layout and flags. It does not identify which
TLAS was bound to GI, read instance bytes, track resource ownership or establish
cross-queue GPU ordering. Multiple TLAS builds on one list are retained, capped
at eight with explicit overflow. Native submission is forwarded unchanged.
The standalone submission ledger and triangle decoder are not connected to
live rendering yet. No sample reuse or speckle correction is enabled.
