# R41 capture findings and R42 change

The two supplied four-part archives were reconstructed by manifest offset,
verified against every part SHA-256 and the complete archive SHA-256, and
extracted locally. Both WPR status files report zero lost events. Both record
Control process 52468 with the R41 DLL hash
`a5b3045001a659e04f7bee9ac083ea48c6ee75c1d6bd8645074765de42cd72df`.

Analysis uses StartUtc + 1 through StartUtc + 19 (18 seconds), excluding WPR
startup and rundown. The log confirms different RR input dimensions and no
primary PT preparation. Present entry intervals during recording average
14.864 ms for DLAA and 19.161 ms for Quality. These are instrumented captures,
not a clean FPS benchmark; recording overhead and different scene workload
must not be mistaken for a measured resolution-only speed comparison.

Matching optimized PDB public and local procedure symbols resolve sampled
user stacks to this repeated path in both recordings:

`renderer -> d3d -> PTGlassRuntime::CopyHook -> PTTableMirror::Scope::Scope -> _Mtx_lock -> ntdll`

Other samples execute descriptor metadata lookup/copy/erase and mutex unlock
paths. The global descriptor transaction serializes these native recording
threads even when their descriptor writes do not touch PT state. This is
concrete evidence of mod CPU synchronization overhead. It does not prove that
this lock is the only bottleneck or predict a particular FPS improvement.
Cumulative timing across threads is not frame time and cannot be added to it.

## Change

Only single-descriptor copies gain a rejection filter before the existing
transaction. The monotonic atomic bitmap contains every address ever assigned
AS metadata, typed-UAV metadata, or a cached source token. An enclosing range
covers all retained mirrored table heaps. All insertion and copy-propagation
paths mark interest before returning to the application. The filter never
forgets an address; collisions and saturation produce false positives only.

If neither endpoint has interest and the destination cannot touch a mirrored
table, the native copy and legacy glass observation still execute, but the
PT descriptor gate and both metadata observers are unnecessary. All other
copies retain the original synchronization and native/observer ordering.
Creation hooks and multi-range copies keep the original path.

This relies on the native D3D12 application's normal synchronization for
conflicting descriptor accesses. The optimization introduces no new allowance
for concurrent reads/writes of the same descriptor. Disjoint copies may run
without the mod's global gate. Snapshot consumers read only tracked tables.
No shaders, queue locks, GPU resource lifetimes or scene-capture policy change.

## Verification and remaining check

The portable differential test compares 100,000 randomized AS descriptor
operations against an independent reference and tests concurrent marking.
Existing AS descriptor tests exercise bulk copies, source reuse and invalid
transactions. WARP tests check retained CPU mirrors, typed-UAV propagation,
source-token invalidation, immutable snapshots and GPU readback. The Windows
workflow also builds the optimized DLL and its matching full PDB.

The `descriptor_untracked` sampled counter measures fast-path use. The first
in-game comparison should run without WPR: fixed scene and settings, PT off/on,
DLAA/Quality, followed by the small collector ZIP. No performance recovery is
claimed before that validation. PT-off geometry capture remains active for
live PT activation; additional costs may remain after this targeted change.

References:
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/copying-descriptors
- https://github.com/microsoft/perfview/blob/main/src/TraceEvent/Parsers/KernelTraceEventParser.cs
