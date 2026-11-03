# Combined GI scene packet integration

This checkpoint replaces separate per-layer developer tests with a combined
CPU integration test. It does not request a new game run or ship a new game DLL.
The R303 user capture already matched all 30,565 observations across four instance
snapshots to submission-specific descriptors, with zero missing/unavailable/type
errors. It does not prove live allocation identity, attribute bytes or materials.

## Reused PT implementation

Recovered unchanged from Frooglemcdoo/Control-FG commit
2f314e8992aec822f13cafbf7efa8bb55dc3915c (R103):

- src/gi_reuse/pt_native_binding.h
- src/gi_reuse/pt_scene_admission.h
- src/gi_reuse/pt_shader_table_adapter.h

These preserve native index/attribute roots and local constants, validate material
and buffer bounds, distinguish opaque/alpha shader identifiers, and refuse partial
private-record publication. Native binding rows are 64 bytes; the audited native
SBT records are 96 bytes. The existing ordering adapter and geometric-normal
helper are used without rebuilding their algorithms.

The R103 live route was also read. Its ReadDispatch checks a two-ray reflection
layout, including a 64-byte miss table; ReadIdentifiers demands all four exports.
Those live guards must NOT simply be installed for the GI producer. The static
GI audit specifies contribution 0, geometry multiplier 2, mask 1 and miss index 0.
That preserves row spacing but does not prove the reflection dispatch/writer
route, pipeline, exports or scheduling are the same for GI.

## Combined adapter

scene_hit_packet.h resolves the TLAS and BLAS through OrderedASMetadata using
allocation tokens and an explicit ordering cut. It requires matching descriptor
revisions, instance array indexing, immutable byte views with a common snapshot
epoch, validated native local-root correspondence and known shader identifiers.
It decodes 16/32-bit triangle indices and padded float3 positions, applies the
instance transform, preserves winding in the geometric normal, and returns the
material ID and alpha-export classification. GPU/process addresses are never
dereferenced. Failure leaves no partial result.

This is an offline consumer of caller-supplied evidence, not a mechanism for
establishing that evidence. Shared byte views must come from completed captures;
allocation tokens, epochs and pipeline identifiers must come from the live host.
The combined adapter does not yet run in the game. It cannot fix or bypass the
current AS owner's 512 MiB retention limit. Same resource identity is not proof
of active alias/suballocation identity or unchanged GPU contents.

The private-record adapter currently rejects per-geometry transforms. That
limitation is explicit rather than silently dropping a transform. It reports
alpha-tested shader classification, not a completed alpha visibility test.
Accepted-hit identity, material texture/alpha evaluation, unbiased GI sampling
and native lighting/RR integration remain required. A computed normal is not
proof that a native ray accepted this triangle. No reuse-ready flag is set.

## Validation

`tools/test-gi-scene-hit-packet.cpp` exercises the full chain using owned synthetic
bytes, a real ordered build/copy/TLAS journal and native-format SBT rows:

- 16- and 32-bit indices, padded positions and exact byte extents;
- copied BLAS lineage and allocation-token lookup;
- instance array index distinct from application InstanceID;
- translated, nonuniformly scaled, mirrored placement and preserved winding;
- material ID/local-root/export agreement;
- stale versions/epochs, wrong owners, truncated tables, invalid indices,
  degenerate triangles and competing queue writers.

Portable warnings-as-errors and UBSan passed. Windows validation is included in
the existing foundation workflow, not delegated to another user game capture.

## Static native writer check

The previously supplied d3d_rmdwin10_f DLL is available locally. GNU objdump
confirms the R103 documented 0x34378 binding-row increment of 0x40 in the
0x33a40 writer. This is only a checked instruction, not a completed GI route
or full writer audit. Further native integration must retain hash/version gates.
