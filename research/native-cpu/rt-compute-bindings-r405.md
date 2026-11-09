# R405: batch current buffer SRV bindings inside dynamic updates

R404 provider updates consumed 0.801521 ms/timed job, 94.64% of the dynamic
stage. This does not establish how much of that time is descriptor copying.
The candidate removes actual driver calls in an eligible path; game speedup
and eligible coverage remain unmeasured until the next run.

## Native contract and scope

Only renderer call A6B6A -> 1DB750, inside the known skinned provider A67B0, is
redirected. Hash-check both DLLs, verify the full shader application body and
D3D 12E50 body, and check the three renderer IAT targets. The wrapper preserves
seven shader snapshots, current thread device retrieval, setShaders, live
per-stage presence checks, and native handling of unsupported constant tables.

For constant-table types 5/6, native use calls D3D 12E50 then 132D0. The first
routine updates current constant buffers via 10570, binds CBVs/root CBVs, then
copies texture SRVs, buffer SRVs and samplers. The owned subset rejects tables
with any textures/samplers. It retains 10570 exactly once, reproduces both CBV
loops, combines current buffer SRV copies, then calls native 132D0 for UAVs.
Compiled metadata is read on every invocation, never retained or modified.

Scope bounds: 2–36 total buffer descriptors, at most 36 ranges, valid SRV slots
0–35, nonnegative provider IDs/offsets, at most 16 records per CBV list and
CBV slots 0–8. Unsupported metadata returns before any side effect. The native
SRV destination base is snapshotted before CBV binding, as in the original.
Null source handles use the current native null-buffer descriptor. Duplicate
or overlapping copy ranges use sequential CopyDescriptorsSimple. Source
handles are resolved anew each invocation; no persistent resource cache.

CopyDescriptors is an immediate CPU operation supporting sparse ranges. Source
and destination heap types must agree, and write/read/write ranges cannot
conflict. See Microsoft documentation:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/copying-descriptors
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-copydescriptors

The active PTGlass CopyDescriptorsSimple hook also feeds lighting shadows,
and the GI dual bridge has a copy observer. When either of these
thread-local observer scopes is active, the original entire shader application
runs. Batching must not silently bypass their notifications. No additional
native update, resource transition, compute dispatch or AS build is removed.

## Validation

The tests execute relocated original 12E50 and 1DB750 machine code with exact
callback mocks. 4096 cases compare descriptor operations, CBV data/root state,
null buffers, duplicate/overlap fallback, shader-stage arguments, current stage
presence after mutation, native fallback and constant-update count. Unsupported
texture/sampler metadata is rejected without calling the update helper.

Separate WARP test runs a shader reading eight actual buffers. It compares
individual descriptor copies with batched copies, and checks a wrong-source
negative control. This validates GPU-visible descriptor contents, not RTX
performance. Production instrumentation reports eligible tables and sampled
application time; it must be read together with the user's FPS report.

The separate OpaqueReflectionRuntime header is not included by this build. If
that module is enabled in future, its descriptor observer requires the same
fallback guard before batching is supported alongside it.
