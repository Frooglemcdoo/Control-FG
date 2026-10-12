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

The first combined path was validated at remote commit
`3c891ab045fc81a5e2b4f11aeaf11716306b59b5`: all six Windows workflows passed.
Run 38308605681 executed the geometry cases on Microsoft Basic Render Driver,
including translated and mirrored/nonuniform-scaled instances. This was reference
driver execution, not RTX hardware or Control integration validation.

## Native writer ordering and the next connection

Inspection of the existing hash-locked Steam d3d DLL establishes:

| RVA | Native operation |
| --- | --- |
| 354A2 / 354A5 | Load the state object's first pointer and SetPipelineState1 |
| 357C6 | Optional inline table writer |
| 358E4 | Record DispatchRays |
| 359D9 | runJob through import 5D650, after dispatch recording |
| 35A14 | Ray queue Wait (slot 15) for the table-copy fence |
| 374A1 | Worker calls the same table writer 33A40 |
| 313FB | waitJob through import 5D648 |
| 314C9 | Copy queue Signal (slot 14), after the job join and copy submission |

Thus the SBT is not necessarily CPU-ready at DispatchRays, or even at its queue
submission. A latest-ready-table lookup would select the wrong generation. The
live connection needs a dispatch reservation whose worker publishes before its
job returns, with the reservation linked to that exact job and buffer generation.
The native wait protects the native copy work; it must not be assumed to protect
arbitrary extra GPU work without matching its queue position and resource states.

`pt_native_shader_table.h` now reconstructs the 96-byte native hit records from
CPU writer inputs. State+0 identifies the PSO; state+20 holds four identifier
pointers indexed by `2 * geometryType + rayType`. The next 56 bytes are copied
from binding+8. Only the recovered two-ray layout is admitted. The one-ray writer
still reserves a second row, so treating it as a compact one-ray table is wrong.
No native upload buffer mapping or GPU readback is needed for this decoder.

The DXR fixture now reserves a table, submits behind a ready fence, then has a
worker construct and upload the private records. Valid test rays are published
last; failure leaves invalid rays that cannot execute TraceRay. This models late
publication and safe failure. The late-write and rejected-worker cases passed on
the reference DXR driver at remote commit
`794b90c83692e82c33d876cf67ae87f4b7bdaa45` (run 38309859057). All six workflows
passed. Neither result is a live Control rendering test.

## Dispatch/job routing hooks

`pt_native_job_runtime.h` now connects the existing native dispatch hook to the
exact runJob call at 359D9 and the existing worker writer hook. The dispatch
reader validates the native PSO, double-buffer selection/counter, complete table
layout, two-ray shape, and writer arguments. Scheduling registers a unique
ticket before the original runJob call. Writer publication happens after the
native fill returns but before the worker returns to the job system.

`pt_native_job_route.h` tracks the 16 native job slots with a monotonically
assigned ticket serial, exact writer arguments and one claim per job. A stale
completion cannot clear a reused slot. The routing mutex is never held while a
consumer callback executes. Error-state preservation applies at each boundary.

This bridge is inert until a session-lifetime renderer consumer registers. No
consumer is registered yet. The inline writer and native zero-identifier debug
mode are intentionally unsupported and retain native behavior. The public
registration contract does not allow replacing a consumer with pending jobs.

Still required: the consumer must reserve its private dispatch/output resources,
capture the selected TLAS and descriptors, retain all GPU dependencies, build the
table using the writer callback, publish behind the correct queue dependency,
and retire resources on a real fence. The native observer's 120-second registry
is not a production resource owner. Final lighting composition also remains
unimplemented. Do not enable private rendering solely because routing tests pass.
