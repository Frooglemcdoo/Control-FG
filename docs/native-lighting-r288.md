# R288 — GI geometry integration test

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

Reports start with GI_METADATA_R288 and distinguish OBSERVING from INCOMPLETE.
They include build/copy counts, retained buffer bytes, resolved/missing ranges and
an incompleteness reason. Neither status means the replacement renderer is active.
Unknown command lists, concurrent submissions, multiple queues, ambiguous AS-list
batches or unsupported operations invalidate metadata claims without modifying
native rendering. Dynamic updates check source lineage and descriptor invariants;
unchanged index-buffer contents are still an unverified API requirement.

This package combines startup, traversal, geometry update and ownership coverage
in one lightweight session. Existing R285 image captures remain sufficient for
lighting analysis; this test requests no new image or buffer dump.

R288 replaces unused startup buffer entries with recent observations and protects
buffers once a geometry range resolves to them. The original limits remain.
Reports add geometry_owners and cache_evictions and specific failure causes.
Evicted buffers may be missing if referenced later without another observation;
that remains explicit missing coverage, never permission for an unsafe GPU read.

R288 observes base D3D12 committed/placed/reserved resource creation and native
transition/UAV/aliasing barriers, in addition to copies. Default-heap buffers
have eviction priority over unreferenced upload buffers; referenced buffers remain
protected. The same 2048-object/512 MiB logical-extent limits remain. This is a
bounded diagnostic inventory, not exhaustive lifetime/state/aliasing validation.
Creation through newer device methods can still need separate coverage.

Command-list journals now grow on the heap to a hard ceiling of 65536 events,
retaining event order and immutable geometry snapshots. Allocation and hard-limit
failures remain explicit. journal_peak, creation_observations and
barrier_observations distinguish capacity from observation coverage. Multi-queue,
ambiguous multi-list submission and unsupported geometry remain fail-closed.
No new barriers, GPU reads, lighting changes or replacement GI are enabled.
