# Glass lighting binding contract

Evidence: user-supplied Steam `d3d_rmdwin10_f.dll`, SHA256
`cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5`.
No compatibility claim for other DLL hashes. Run
`python tools/audit-pt-glass-native-bindings.py <native-dll>` to verify the
source identity and instruction evidence. Native binaries are not included.

## Runtime root signature

Factory RVA `0x18c10` constructs an eight-parameter version 1.0 root signature.
The return value is installed at renderer-device offset `0x70` by `0x2b049`.
The runtime factory is authoritative: `getRootSignatureCSAsString` omits the
material descriptor table that the runtime adds.

| Root parameter | Binding | Table offset / count |
|---|---|---|
| 0 | b4..b8, space0 | 0 / 5 |
| 0 | t0..t35, space0 | 5 / 36 |
| 0 | u0..u7, space0 | 41 / 8 |
| 1 | s0..s3, space0 | 0 / 4 |
| 2..5 | b0..b3, space0 | root CBVs |
| 6 | s0..s63, space1 | 0 / 64 |
| 7 | material t0 onward, space1 | 0 / 244000 |

All visibility is ALL; root flags are NONE. The test-only fixture reproduces
this factory. Production must borrow and verify the actual captured root.

Secondary lighting overrides exactly b6 (table index 2), t28 (33), and u0 (41).
It requires observed b4/b5 and t1..t23. It retains b0..b3, samplers, and materials.
The staging component will not publish until the native inputs and the three
explicit secondary overrides are supplied for the current recording.

## Native descriptors and thread selection

`cycleDescriptorTableCS` (`0x1bd50`) allocates 49 resource descriptors through
`0x1aa00`. Descriptor-manager offsets are:

| Offset | Value |
|---|---|
| 0x30 | resource table GPU handle |
| 0x38 | sampler table GPU handle |
| 0x40 | resource table CPU destination handle (shader-visible heap) |
| 0x48 | sampler table CPU destination handle |
| 0x50 / 0x58 | resource / sampler heap provider pointers |
| 0xf8..0x110 | desired b0..b3 GPU addresses |
| 0x118..0x130 | cached bound b0..b3 GPU addresses |
| 0x138 / 0x140 | cached global material / sampler GPU handles |

A provider contains the heap pointer at 0, CPU base at 8, GPU base at 16.
Allocator `0x1aa00` reserves the first `0x3b920` descriptors for materials and
allocates transient tables above that prefix. Heap rollover is possible. The
staging publisher rejects a destination in another heap; it must not silently
switch heaps while retaining native material-table handles.

Native functions read the compiler TLS vector at `gs:[0x58]`, then index it with
module TLS index at RVA `0x1115fc`. Block+0x20 supplies the thread descriptor
manager; a null manager falls back to global pointer RVA `0x111c10`. Block+8 is
the command context (fallback `0x111c18`), context+0x18 is the command list.
This evidence does not authorize using an unrelated dynamic TLS API lookup.

`ConstantTable::use` (`0x10770`) routes compute inputs through `0x12e50` and
`0x132d0`. The former creates CBVs directly into the shader-visible table and
copies SRVs from CPU source descriptors. A runtime adapter therefore needs to
mirror native CreateConstantBufferView and descriptor-copy writes while this
specific glass table is populated. Copying the live shader-visible table back
to staging is invalid under D3D12's descriptor-copy contract:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-copydescriptorssimple

`0x1c180` binds table roots 0/1, b0..b3, then global roots 6/7. The scoped custom
binding changes only root 0 and PSO and restores both before returning. It must
run at the verified glass dispatch, on the same recording thread and list.

## Validation and remaining integration

The WARP test executes native/custom/native dispatches using separate outputs,
a distinct custom PSO, a material-table input, and a root-CBV input. Expected
readbacks are 30 / 142 / 30. It also rejects shader-visible copy sources,
native-table overlap, material-prefix writes, stale generations, and mutation
after publication. Resource lifetime is tested separately against a blocked
GPU queue, including resolution changes and allocation exhaustion.

These components are not installed in the live game. Remaining wiring is the
narrow native descriptor-write adapter, actual list-generation/submission
callbacks, secondary ray production, and transport/composition into glass.
The source and CI checks preserve the active rendering baseline. Passing WARP
and PSO checks is not visual proof of the final glass effect.
