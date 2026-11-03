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

## Verified Windows checkpoint and lifecycle audit

Foundation run 41640744822, job 125933318948, passed on Windows at remote
commit a235045852d36f79deda32701b20772559685c2d. The combined scene-packet
executable passed under MSVC /W4 /WX, alongside existing CPU and WARP tests.
This is not a live-game visual validation.

`tools/audit-gi-native-lifecycle.py` now checks the supplied DLL by SHA-256,
PE executable-section mapping, direct-call opcode and signed relative target.
`docs/gi-native-lifecycle-audit.json` records all 27 R103 creation/release sites
and two binding-writer sites. All 29 match; 58 opcode/target mutations are
rejected. The tool never modifies the DLL. It also checks the 64-byte writer
row increment. These are listed sites, not a proof of exhaustive callers.

Static inspection of native release at 0x25f00 shows that it reads the wrapper's
resource at offset 0, calls its vtable slot 2 at 0x25f20, and clears wrapper
fields 0, 0x10, 0x18 and 0x40 before returning. Thus a future retirement bridge
must snapshot/retain the identity before forwarding release, not read the
wrapper afterwards. The old PT BeforeRelease/AfterRelease sequencing supplies
a reusable design, but it is not connected to the current AS registry yet.

The current registry retains historical owners without retirement. Raising its
512 MiB cap alone would postpone saturation rather than fix that lifetime.
The next live integration must remove retired identities from new lookups while
preserving independent leases for pending GPU work and old scene cuts. Native
release does not itself prove GPU completion, alias validity, or that every
AS allocation passes these callsites. No release hook or larger cap is enabled
by this checkpoint; replacement GI remains disabled.

## Retirement API implementation

ASResourceOwners now has token-checked Retire. It removes the exact generation
from new address lookups, returns the retained resource/heap to the caller,
reuses vacant index slots and never resets its monotonically increasing token.
Existing leases stay independent. Registry byte accounting describes indexed
owners, not all outstanding leases; those must remain separately bounded.

The Windows runtime fixture now exercises stale and duplicate retirement,
re-admission of the same pointer, surviving adjacent index entries, placed heap
retention and 1,000 retirement/reuse cycles. This checks COM lifetime and registry
consistency, not an actual pending DXR command. The API remains disconnected from
native release: current journals retain tokens rather than AS leases, so enabling
retirement now could invalidate resources still used by submitted work. Submission
lease retention and a verified native lifecycle bridge are required together.

## R304 submission integration — no additional user capture requested

Remote code 610b4535a8bb996d8b7a190e130bf21243c3d1c3 adds distinct recorded
AS leases and retains each AS-bearing journal through the observer's private
submission fence. Resetting command-list metadata cannot release submitted roots.
Same-queue replay gets its own retained submission. Completed references are
released outside the state lock; uncertain/failed submissions remain pinned.
The pending ring is bounded at 256 entries. Native release remains disconnected.

Windows foundation run 41643117186 / job 125941390697 passed, including an
actual blocked WARP queue, replay, simulated registry retirement, list metadata
reset and fence-gated lease release. WARP executed empty lists, not DXR builds.
The combined geometry/material test and 1,000 registry reuse cycles also passed.

Existing R303 evidence already supplies the scene mapping: 30,565 observations,
zero missing mappings across four samples. Repeating that two-minute run would
not prove the missing traversal-lifetime or GI writer contracts. Therefore the
R304 package is an integration checkpoint, not a request for another capture.

Remaining live gaps are explicit: BLAS references reachable through TLAS,
DispatchRays resource uses, native wrapper lifecycle coverage, GI-specific writer
and shader export selection, captured attributes/material textures/alpha, and
sampling proposal validity. A new user run must answer a named gap that the
existing PT sources, supplied DLLs and archived captures cannot resolve offline.

Full build 41643117332 / job 125941391200 passed at the same code commit.
Artifact 13234011791 was downloaded and verified against its SHA-256 digest;
the nested package checksum, ZIP integrity, 74-file layout, R304 lease marker
and exact-index shader sidecar passed. ControlTOH-R304.zip is 4,216,584 bytes,
SHA-256 66bd70705900359195a3d34650f459b8e312cfdfd878cb0e564ee0ae473b4af1.
Saved package: libfile_1dc1a82da3988191b0532298f34d7424. No user run requested.

## Complete traversal closure admission

scene_traversal_closure.h now validates every reference in a complete TLAS
instance array at one ordered version. It preserves masked BLAS references,
deduplicates identical identities, rejects conflicting tokens/revisions, validates
null slots and refuses partial arrays or epoch mismatches. Failure publishes no
partial closure. Limits are 65,536 instances and 8,192 unique referenced BLAS.

The combined CPU fixture exercises duplicate/masked/null instances, complete
versus partial arrays, conflicting identities and epoch rejection for both index
widths. Portable warnings-as-errors and UBSan passed. The host still must obtain
version-matched instance bytes and allocation identities and attach real leases;
this helper alone does not make native release safe. Existing R303 metadata-only
matching is not falsely promoted to that proof. No additional user run requested.

Windows foundation run 41652270346, job 125973246312 passed at remote commit
de92569d4aed96fc4c1408039663a449cd770661. Local and remote trees match
99746349f6e7804b9432e603a0d06e29a0abdc1f. No new package delivered and no
user capture requested for this offline closure-admission change.

## Atomic traversal resource retention

Recovered PT Runtime::PrepareLease validates allocation tokens and retains their
resources under the same owner mutex. ASResourceOwners::ResolveBatch now applies
that transaction to a bounded set of expected address/token pairs. It allocates
output storage before locking, rejects missing/ambiguous/stale identities, retains
resource plus placed heap together, and publishes only after the entire set passes.
Rejected candidates release their COM references outside the registry lock.

RetainTraversalClosure connects the complete-array admission result to this batch
acquisition. Host code still supplies a validated closure and preserves metadata
and byte provenance. The transaction excludes registry retirement, not native GPU
writes or alias changes. The Windows fixture covers late-member failure rollback,
exact identities, placed heap retention and leases surviving registry retirement.
No live DispatchRays hook is enabled by these helpers; native retirement remains
disconnected. No new user capture is requested.

Windows foundation run 41653889808 / job 125978957102 passed at remote commit
a0361000da01ee79f4a15f2c553ba09bdeebc8a9. This includes atomic AS set
acquisition, complete traversal admission, blocked-queue replay/reset and the
existing CPU/WARP suite. Windows max-macro compatibility was corrected in the
recovered PT headers without changing their algorithms. Local and remote code
trees match 9b5aaff0bff0ac889968fedb18bc0d46edb62e92. No user run requested.

## Native GI route and one-ray table recovery

The supplied renderer hash now has a dedicated static audit in
`tools/audit-gi-native-route.py`, with results in
`research/gi-source-r267/native-gi-route.json`. Seven imported callsites establish
GI setup `(1,2)`, rayGenMain, missMain, closestHitMain and anyHitAlphaTest. The
native writer computes `2-rays` reserved rows and identifiers at
`state+0x20+8*(ray+2*type)`. All nine writer instruction checks and seven mutated
import targets were tested against the supplied DLLs. This is a static route
proof, not live state-object/worker/queue provenance.

NativeGITable reconstructs only the active even rows with 56 local-root bytes,
preserving the native 96-byte stride and two-row geometry spacing. Reserved rows
are explicitly inactive; their zero-filled storage is not claimed to match native
padding. The combined scene-hit fixture now consumes this reconstruction instead
of a hand-written shader record, for both index widths and alpha-tested material.

The existing renderer observation hooks track all required GI exports in a
thread-local route. Every begin resets the route and previous ray-generation name;
a missing or incompatible binding fails recognition. GI_NATIVE_ROUTE logs are
bounded and observation-only. They neither gate nor replace native rendering.
The native writer itself is not yet hooked, so the CPU reconstruction does not
supply a live GPU table or ownership proof. No additional user run is requested.
