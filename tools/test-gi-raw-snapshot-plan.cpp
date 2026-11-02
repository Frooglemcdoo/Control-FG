#include "../src/gi_reuse/raw_snapshot_plan.h"
#include <cassert>
#include <iostream>
using gi_reuse::RawSnapshotPlan;
int main() {
    RawSnapshotPlan p;
    assert(RawSnapshotPlan::Make(512, 258, 6, p));
    assert(p.sourceOffset == 256 && p.prefixBytes == 2 && p.copyBytes == 8);
    assert(RawSnapshotPlan::Make(512, 2, 266, p));
    assert(p.words == 67 && p.groups == 2 && p.prefixBytes == 2);
    assert(!RawSnapshotPlan::Make(266, 2, 264, p)); // Rounded tail outside resource.
    assert(p.copyBytes == 0 && p.groups == 0);
    assert(!RawSnapshotPlan::Make(512, 512, 1, p));
    assert(!RawSnapshotPlan::Make(512, 0, 0, p));
    assert(!RawSnapshotPlan::Make(UINT64_MAX, UINT64_MAX - 1, 1, p));
    assert(!RawSnapshotPlan::Make(UINT64_MAX, UINT64_MAX - 1, 4, p));
    assert(!RawSnapshotPlan::Make(UINT64_MAX, 0, RawSnapshotPlan::MaxBytes + 1, p));
    assert(RawSnapshotPlan::Make(RawSnapshotPlan::MaxBytes, 0, RawSnapshotPlan::MaxBytes, p));
    assert(p.groups == 65535);
    // Exhaust every subrange of short resources against an independent byte model.
    for (std::uint64_t owner = 1; owner < 80; ++owner)
        for (std::uint64_t offset = 0; offset <= owner; ++offset)
            for (std::uint64_t bytes = 0; bytes <= owner + 1; ++bytes) {
                const bool valid = bytes && offset + bytes <= owner &&
                    ((offset + bytes + 3) / 4) * 4 <= owner;
                assert(RawSnapshotPlan::Make(owner, offset, bytes, p) == valid);
                if (valid) {
                    assert(p.sourceOffset + p.prefixBytes == offset);
                    assert(p.sourceOffset + p.copyBytes <= owner);
                    assert(p.copyBytes >= p.prefixBytes + bytes);
                    assert(p.groups * 64 >= p.words);
                }
            }
    std::cout << "PASS raw snapshot alignment, uint16 tails, overflow, bounds and dispatch limit\n";
}
