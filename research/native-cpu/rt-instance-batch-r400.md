# R400: group-scoped shared geometry/material preparation

## Evidence and decision

User reports R399 looks correct and has no visible FPS improvement. Entire uploaded
RT-Assembly-R399.zip contains probe-20261109-165212-711-63360.log (112,804 bytes).
All three hooks install. Latest assembly summary: 75,235,351 instances, 98,388,812
geometry records, 71,644,778 static / 3,590,573 skinned / zero virtual providers,
324,094,865 getter calls avoided. Latest loop summary: 9,728 loops, 72,251,562
objects, 71,806,293 instances, 3,435,651 dynamic updates, 23,251,152 us (2.390127 ms
mean). Selection: 2,816,958 us / 9,728 (0.289572 ms), zero fallback. No error-like
lines in the log. Counters have different flush boundaries. R398 loop mean was
2.080496 ms; different gameplay prevents a controlled regression/gain claim.

Prior dirty-writer and grid audits do not prove a complete cross-frame generation
or lifetime protocol. Do not introduce pointer-keyed persistent caches. The native
0x800-object branch instead supplies a useful smaller reuse boundary: one ready
provider, one material owner and variant zero, then count consecutive transforms
with no intervening readiness/dynamic-update callback. Preserve this boundary.

## Change and limits

TryAssembleBatch requires >=2 transforms, the byte/vtable-verified static provider,
nonnegative geometry count, and sufficient existing capacity in all three output
vectors for the WHOLE group. The check uses uint64 multiplication and low-30-bit
capacity, rejects used>capacity, and makes no output changes on rejection. Rejected
groups invoke the existing assembler per transform. Skinned/unknown providers stay
on that path. No additional allocation, worker or persistent pointer cache.

Prepare first instance normally. Copy its already-resolved geometry/material rows
for subsequent transforms; copy its instance flags/mask/current BLAS address,
replace transform bits and low-24-bit contribution (preserving upper flags), and
advance prefix exactly as native. Counts/order/output bytes remain identical.
Only pure verified getters/material reads are omitted; the capacity prerequisite
ensures no native allocator callback occurs while replaying the group. Native
object/material lifetimes must cover the synchronous group, as required by the
original loop. Concurrent unsynchronized mutations within a group remain outside
the supported renderer contract. Every later group reads fresh state.

Native recording A8B55-A8B98 still patches each geometry record's material ID from
its corresponding native material pointer. No table compaction or shared SBT index
is introduced. Every native instance and per-instance geometry row is retained.

Runtime batches telemetry with actual instance count and records skipped geometry
preparations. Loop telemetry reports batch_groups and reused_instances; both reuse
counters are needed to establish coverage, not an FPS gain. No existing capture
export provides reliable per-group multiplicity, so benefit remains unmeasured.

## Validation

4,096 additional cases execute original A8CE0 repeatedly and compare all output
records, counts, capacities and prefix with production batch logic. Vary group
size 0..8, geometry count 0..8, nonempty vectors, capacity high bits, growth fallback,
unknown and redirected bindings, signed BLAS states/offsets, transform NaN bits,
cull/alpha flags and prefix wrap. Failed batch attempts must leave output untouched.
All 8,192 exact getter/assembler and 4,096 generic assembler comparisons remain.
The 4,096 outer-loop oracle now enables the batch callback and compares expanded
callback order, transforms, prefix and dynamic updates against the original loop.
Native group scheduling/material publication are not simulated by this leaf test.

This is group-local reuse, not the completed incremental persistent scene design.
