# R410: incremental material descriptor heap refresh

R409's 256 samples after the first 64 contained 244 ordinary allocations and
12 refill/change cases. Ordinary lock hold was 66 us total; refill hold was
6579 us (548.25 us each). Refill hold contributed 83.60% of measured wait,
hold and release. No sampled pool growth. This is CPU-inclusive evidence,
not proof that these operations caused the worst frame intervals.

## Native contract and audit

Audited D3D SHA256 cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5.
Export setMaterialDescriptor at 1A9C0 stores an eight-byte CPU handle at
manager+1F8+index*8 and increments manager+8, including same-handle writes.
It is a 13-byte leaf without a pdata entry. Renderer C819C/C81E4 call it
from C7BC0; C83A6 publishes the material descriptor count through 1A9D0.
The constructor 19540 initializes the source table to null descriptors.

Allocator 1AA00 holds manager+18, chooses a heap whose retirement fence has
completed (or creates one), resets its cursor, and copies max(currentCount,
heapPreviousCount) individual CPU descriptors through the single virtual call
at 1AC12. It then stamps heap+30 with manager generation and heap+38 with
current count. Full allocator bytes and setter bytes are checked at startup.

## Replacement

Patch only the setter leaf and seven-byte CopyDescriptors call. Keep allocator
control flow, completed-fence checks, native generation stamps, 49-slot
reservations and heap lifetime unchanged. The setter performs identical native
writes and marks a 256-slot page, even when the handle value is unchanged.
A small SRW lock serializes tracking/publication with refresh; it never acquires
the native allocator lock, avoiding inverse lock ordering.

Bounded state: 954 page versions and 64 heap identities; no allocation per
publication. Identity includes heap record, COM heap object, CPU base, native
old generation/count and tracker epoch. First use, count changes, generation
gaps, manager changes, unknown identities and cache overflow use full copies.
More than half the table dirty or over 32 merged ranges also uses full copy.
Shrinking tables retain native max(old,new) tail copying. No comparison of
source handle equality is used to infer descriptor content equality.

Sources remain the original non-shader-visible CPU descriptors. Changed page
ranges copy to the same native destination. No GPU heap is read on the CPU.
Microsoft contract: https://learn.microsoft.com/en-us/windows/win32/direct3d12/copying-descriptors

The publication contract is essential: source content changes must be
republished through the native setter, as required for native generation-based
binding invalidation. Unexpected generation changes invalidate all history.
The added publication tracking itself has CPU cost; reductions in descriptor
copy counts are not automatically FPS improvements.

## Validation

Portable randomized full-copy oracle: 2000 refreshes over four recycled heaps,
repeated publications, generation gaps/wrap, identity changes, growth/shrink
and bounds. WARP test executes the actual runtime copy/publication hooks via
patched seven-byte virtual-call and 13-byte setter fixtures. GPU outputs across
1024 descriptors compare incremental refresh against a full copy, including
same-handle in-place descriptor rewrite and a zero-copy repeat.
R409 lock probes are no longer installed; setup/frame diagnostics remain.
