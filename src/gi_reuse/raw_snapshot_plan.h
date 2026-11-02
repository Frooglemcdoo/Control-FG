#pragma once
#include <cstdint>
#include <limits>

namespace gi_reuse {
// ByteAddressBuffer loads operate on complete DWORDs. Include alignment padding
// only when it remains inside the retained resource; never assume heap padding.
struct RawSnapshotPlan {
    std::uint64_t sourceOffset = 0;
    std::uint32_t prefixBytes = 0, copyBytes = 0, words = 0, groups = 0;
    static constexpr std::uint64_t MaxBytes = 16ull * 1024 * 1024 - 256;
    static bool Make(std::uint64_t ownerBytes, std::uint64_t offset,
                     std::uint64_t bytes, RawSnapshotPlan& result) noexcept {
        result = {};
        if (!bytes || offset > ownerBytes || bytes > ownerBytes - offset) return false;
        const auto end = offset + bytes;
        if (end > std::numeric_limits<std::uint64_t>::max() - 3) return false;
        const auto roundedEnd = (end + 3) & ~std::uint64_t(3);
        const auto start = offset & ~std::uint64_t(3);
        if (roundedEnd > ownerBytes || roundedEnd - start > MaxBytes) return false;
        const auto count = static_cast<std::uint32_t>((roundedEnd - start) / 4);
        result = {start, static_cast<std::uint32_t>(offset - start), count * 4,
                  count, (count + 63) / 64};
        return true;
    }
};
}
