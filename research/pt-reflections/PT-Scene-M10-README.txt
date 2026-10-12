Control PT M10 - RETAINED SCENE CANDIDATE INTEGRATION

This is still a diagnostic build. It does not enable full PT or change the image.
M9 completed but made zero ownership attempts; it did not pass that gate.

1. Close Control and back up dxgi.dll.
2. Replace only dxgi.dll beside Control_DX12.exe. Keep existing shader files.
3. Load the entrance with RT ON and PT OFF. Move briefly, then leave the game
   running THREE MINUTES before closing it.
4. After closing, run Collect-PT-Scene-M10.cmd. Send its results ZIP.

M10 retains verified buffer references at creation, links preceding BLAS geometry
observations to those retained allocations, and checks sampled native bindings
against geometry and bounds. It counts ambiguity, missing allocations, budget
limits and invalid bounds separately. It does not submit GPU work, modify GPU
states, or use candidate records for rendering.

Extra retained resource memory is capped at 512 MiB (64 MiB per resource).
Tracking stops after 120 seconds; retained references are then released. Resource
retention and CPU tracking can affect loading/frame times during that interval.
The registry summary is independent of the raw-event recording quotas.

A candidate is not proof of TLAS/SBT correspondence or GPU submission safety.
Those remain required before the private path tracer can render Control's scene.
Requires the verified Steam game DLL and existing mod installation.
Rollback: close Control and restore your backed-up dxgi.dll.
