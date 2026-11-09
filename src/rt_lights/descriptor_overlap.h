#pragma once
#include <cstdint>
namespace rt_lights {
struct DescriptorSlice { unsigned tableIndex = 0, sourceSkip = 0, count = 0; };
// Intersect a copied range with the private table. Avoid endpoint arithmetic
// overflow and preserve source offsets when a copy begins before the table.
constexpr DescriptorSlice DescriptorOverlap(std::uint64_t table, unsigned slots,
    std::uint64_t destination, unsigned count, unsigned step) noexcept {
    if (!step || !slots || !count) return {};
    if (destination < table) {
        const auto delta = table - destination;
        if (delta % step || delta / step >= count) return {};
        const auto skip = static_cast<unsigned>(delta / step);
        const auto remaining = count - skip;
        return {0,skip,remaining < slots ? remaining : slots};
    }
    const auto delta = destination - table;
    if (delta % step || delta / step >= slots) return {};
    const auto index = static_cast<unsigned>(delta / step);
    const auto remaining = slots - index;
    return {index,0,count < remaining ? count : remaining};
}
}
