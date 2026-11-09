# Resolved scene-query contract

This audit continues the R392/PTLAS integration investigation using the uploaded
RT-Scene-Probe(1).txt. It does not enable a replacement rendering path.

## Verified findings

The runtime target at renderer RVA 0x165520 is the vtable 0x634CB8 slot +0x50.
The executable audit verifies that slot in the hash-locked renderer and checks
32 instructions in the dispatcher, traversal, bounds updater and preparation.

- The dispatcher calls traversal 0x1655C0 with selector 0 conditionally and
  selector 1 unconditionally. Selector 0 is omitted only when include-mask bit
  17 is set and required-value bit 17 is clear. The RT call supplies mask 8 and
  required value 0, so both passes run. These are two spatial collections;
  assigning them static/dynamic labels is not yet supported by this evidence.
- Traversal selects a root through query-object offset 0x68 + selector*0x100.
  It walks child cells, applies convex-volume tests, filters 0x30-byte entries,
  and appends selected object pointers using 0x22730.
- Entries are tested against category mask 0x100 or 0x900 at the RT callsite.
  The un-clipped branch applies (object flags & 8) == 0. When flag bit 2 is
  set, object vtable +0x28 is called for an additional test. Specialized
  traversal helpers also have object-category checks; do not substitute only
  the un-clipped branch's tests for the complete query.
- This is not a side-effect-free query. A cell's byte +0x39 gates a recursive
  bounds refresh at 0x165DC0. That routine combines entry/child bounds and
  writes the cell vectors at +0x00 and +0x10, then leaves +0x39 set to 1
  (the store occurs before the vector stores in machine-code order).
- The native preparation clears counts at context +0x18, +0x78, +0x88 and
  +0x98 before querying and rebuilding. Thus observed zero counts have a
  concrete in-progress explanation. The aggregate log cannot prove which
  phase produced any individual zero.

## Implementation consequences

Do not cache query results based only on camera position, object count, the
context frame counter or repeated pointer values. A stationary camera does not
establish unchanged objects, cell bounds, geometry, materials or residency.
Do not call this query externally or on an arbitrary new worker: it mutates
shared cells and uses native ScratchPad allocation. The existing external
probe did not invoke it and remains read-only.

Replacing the full preparation body requires preserving or deliberately
replacing this bounds-maintenance responsibility, all membership predicates,
output ordering and the downstream geometry/material/resource protocols.
A matching pair of external scalar reads is not an atomic snapshot.

The next unresolved code work is identifying cell invalidation writers and
owner synchronization, then tracing query output through geometry/BLAS/resource
ownership to submission and retirement. The isolated PTLAS validator does not
establish these Control-side contracts. No new user capture is needed for the
static tracing completed here.

## Validation and status

Run audit-query-target.py with the exact renderer and optional probe log.
Requires Python, pefile and capstone. The included JSON is the resulting static
PASS for the supplied renderer and uploaded runtime target evidence. The audit
rejects unknown renderer bytes and incompatible target evidence.

No mod DLL was changed, no new in-game build is included, native preparation
has not been retired, and no FPS improvement is claimed. Keep R392 and its
native ray setting.
