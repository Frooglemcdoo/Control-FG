# Scene dispatch integration checkpoint

M11 remains on hold. Do not request another CPU-only game capture to validate
this change. The next requested user run should exercise visible integrated
rendering, or be preceded by an explicit explanation of an unavoidable blocker.

## Implemented

`pt_scene_dispatch_plan.h` combines a selected TLAS snapshot, exact BLAS geometry,
native table snapshot, allocation generations and shader identifier mapping into
one complete private hit table. Selection checks cover TLAS/table addresses,
their versions and native/private state object identity. It never looks up a
global latest scene and publishes no partial output. Both native ray types keep
their record numbers. Unreferenced rows currently reject the entire transaction.

Index/position tokens are frozen with BLAS metadata; index/attribute tokens are
frozen with native bindings. Identical GPU addresses and even a later identical
geometry observation cannot revive an old allocation identity. The resulting
plan lists the geometry allocation tokens that a runtime host must retain.

Private R16 records allow the exact DWORD round-up of the native logical index
bound only when the retained backing allocation covers it. Native attribute
views may exceed the selected geometry's used vertex range, but must still fit
the retained backing view. Neither case admits arbitrary enlarged bounds.

The existing DXR geometry fixture now feeds its actual BLAS/TLAS inputs through
this combined path before uploading and dispatching the private table. It also
corrects the old first-geometry fixture, whose BLAS used the R32 buffer while its
hit record used a separate R16 buffer containing equivalent values. Source
identifiers in this fixture remain synthetic; this is not a Control frame replay.

## Remaining runtime boundary

This is CPU preflight plus a standalone DXR fixture, not a playable Control PT
implementation. The M11 observer is still diagnostic and its aggregate relation
checks still use a latest CPU snapshot. It does not invoke the new planner.

The live host must select snapshots at the actual native dispatch, retain the
TLAS/BLAS and geometry/table/descriptor resources, prove GPU ordering and resource
states, and retire them after the actual queue fence. COM retention alone does
not freeze mutable buffer contents. It must also create/select the private state
object and bind its matching global/local roots without disturbing native state.

After that, primary PT radiance still needs the material-composition integration
described in entrance-composition-contract.md. Writing fully weighted radiance
into the native diffuse coefficient buffer would apply albedo twice. A scene
adapter or passing fixture must not be described as completing that renderer.

## Validation

Local C++ tests cover complete conversion, wrong dispatch selections, foreign
pipeline mapping, snapshot mutation isolation, conflicting instance routing,
late-record failure, reused allocations and halfword-tail padding. Existing
admission/converter/topology tests and the confirmed runtime hash gate also pass.
Windows build and DXR execution status must be checked separately; a no-DXR skip
is never a GPU pass.
