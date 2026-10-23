# Native hit capture: implementation checkpoint after R117

Status: shader and offline record/table validation implemented. **Not integrated
into the game, not an R118 release, and not a lighting fix.** R117 remains the
last installable test. No runtime hooks or packaged assets change in this commit.

## Evidence and limitation

The R117 log `Lighting-R117-20261023-073404-43132-0.log` confirms readiness and
ray-record offsets 192/256/320 for X/Y/Z. RR success remained 1,792 throughout
all diagnostic modes; RR failures remained zero. The cafeteria screenshots show
orange near-endpoint hits over floor/tables, predominantly negative-Y hits, and
both signs in X/Z. Coordinate sign is partly a consequence of ray direction;
this does not identify a fixture or establish a plane. The session ended in
HIT BANDS, so it does not test RR restoration after returning to BOTH.

## Implemented

`rt_spot_capture_layout.h` is the shared C++/HLSL ABI. A diagnostic ray supplies
the actual biased native origin, normalized direction, full native TMax, selected
light ID, source pixel and frame tag. The adapter must extract these from the
same native pass; no second bias or approximate receiver reconstruction.

`rt_spot_hit_capture.hlsl` traces the closest accepted hit using flags 0, mask 1,
hit offset 0, geometry stride 2, miss index 0. It writes private structured-buffer
records with distance, instance index, application instance ID, geometry index,
primitive index, barycentrics and hit kind. GeometryIndex requires SM 6.5. Its
32-byte payload retains distance at offset zero for the native alpha-test
handler's first-field accesses. This layout compatibility still needs a linked
state-object and GPU traversal test; shader validation alone does not prove it.

The result limit is 4,096 records (320 KiB). IDs remain integers. Misses clear
all identity fields rather than reusing any-hit candidates. The decoder rejects
truncated, oversized, stale-frame, nonfinite and invalid-triangle records. IDs
are scoped to a captured TLAS generation, not persistent asset names. Exact
world hit position is reconstructed from origin, direction and hit distance.

`rt_spot_capture_table.h` is an offline reference for translating identifiers
between state objects while preserving the remaining 64 bytes of each observed
96-byte native hit record. Unknown identifiers reject the complete translation
without publishing a partially modified table. This is not a GPU table copier.

## Required before an installable build

1. Build an independent diagnostic state object with a 32-byte payload limit,
   original native alpha-test code and its original local-root associations.
   Leave native gameplay state objects and shader records untouched.
2. Capture actual native ray inputs and a same-generation TLAS/SBT binding.
   Authenticate buffer ownership, state and lifetime before GPU table copying.
   Translate identifiers into the diagnostic state object; preserve material
   arguments and reject unknown records. Do not dereference GPU virtual addresses
   on the CPU or reuse identifiers from a different state object.
3. Bind private input/output descriptors without changing native outputs. Restore
   root bindings, descriptor tables, PSO/state object after diagnostic recording.
4. Record one bounded capture on request. Observe submission of that exact list,
   signal its actual queue, and export only after fence completion. Reset before
   submit, replay, device removal and shutdown must retain/quarantine uncertain
   resources. No render-thread waits or periodic synchronous readbacks.
5. Join decoded records and provenance into the existing session log, with an
   explicit pending/complete/failed UI state. Validate opaque and alpha-cutout
   geometry, multiple BLAS geometries, misses and restored gameplay on GPU.

## Checks completed locally

- DXC `lib_6_5` compilation with warnings as errors; DXV validation.
- C++ shared ABI offsets, table translation, material-argument preservation and
  late unknown-ID rejection.
- Python binary decoding, exact large integer IDs, world-coordinate reconstruction,
  miss clearing, bounds, triangle validity and frame provenance.
- Existing 132-file baseline guard.

Relevant API references:
- https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
- https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_5.html
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/instanceindex

These checks do not establish that live native geometry has been captured.
