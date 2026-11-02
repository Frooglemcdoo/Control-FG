# R286 — GI geometry integration test

This build adds native build/update/copy metadata tracking and an observed-buffer
owner inventory for the future GI replacement. It does not enable ReSTIR reuse,
replace GI lighting, or claim a speckle fix. Existing lighting settings are preserved.

Copy the package contents beside Control_DX12.exe, preserving the ControlTOH
folders. Close the game and run ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd.
Load your normal save, move around for about one minute and exit normally. Logs
are packaged automatically. No F7, F9, RR capture or stationary camera is needed.
Send the resulting diagnostics ZIP and mention any new stutter or startup issue.
Launching normally leaves the development observer disabled.

The test retains observed buffer resource objects up to a 512 MiB / 2048-owner
limit, and submits completion fences without CPU waits or extra GPU copies.
Retention may delay engine resource reclamation. It is test-only, not a performance
benchmark or a validated production ownership solution. References do not freeze
GPU contents or establish placed-heap lifetime, aliasing or resource states.

Reports start with GI_METADATA_R286 and distinguish OBSERVING from INCOMPLETE.
They include build/copy counts, retained buffer bytes, resolved/missing ranges and
an incompleteness reason. Neither status means the replacement renderer is active.
Unknown command lists, concurrent submissions, multiple queues, ambiguous AS-list
batches or unsupported operations invalidate metadata claims without modifying
native rendering. Dynamic updates check source lineage and descriptor invariants;
unchanged index-buffer contents are still an unverified API requirement.

This package combines startup, traversal, geometry update and ownership coverage
in one lightweight session. Existing R285 image captures remain sufficient for
lighting analysis; this test requests no new image or buffer dump.
