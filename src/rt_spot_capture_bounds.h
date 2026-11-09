#pragma once
#include <cstdint>
namespace rt_spot_capture {
inline bool ContainsGpuRange(std::uint64_t base, std::uint64_t capacity,
                             std::uint64_t address, std::uint64_t bytes) noexcept {
    if (!base || !capacity || capacity > UINT64_MAX - base || !bytes || address < base) return false;
    const auto offset = address - base;
    return offset <= capacity && bytes <= capacity - offset;
}
inline bool NativeHitTableLayout(std::uint64_t address, std::uint64_t bytes,
                                 std::uint64_t stride) noexcept {
    return address && !(address % 64) && stride == 96 && bytes &&
           bytes <= 8 * 1024 * 1024 && !(bytes % stride);
}
} // namespace rt_spot_capture
