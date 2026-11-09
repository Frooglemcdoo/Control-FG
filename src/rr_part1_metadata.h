#pragma once
#include <cstdint>
#include <limits>

// Read-only diagnostic building block. Integrated only for metadata logging.
// Caller must authenticate the renderer and establish a stable read interval.
// Reader provides fault-contained uint64 reads. No raw pointer dereference,
// resource acquisition, Map, barriers, binding, or lifetime extension occurs.
namespace control_rr_part1 {
struct Metadata {
    std::uint64_t renderer{}, owner{}, buffer{}, stride{}, bytes{}, gpuAddress{};
    std::uint64_t elements{}, materialOffset{};
};
enum class Result { Candidate, InvalidArgument, Unreadable, NullPointer,
                    BadStride, BadSize, MaterialOutOfRange };
template<class Reader>
Result ReadMetadata(Reader&& read, std::uint64_t renderer,
                    std::uint32_t materialID, Metadata& out) {
    out = {};
    if (!renderer || materialID > 65535u) return Result::InvalidArgument;
    Metadata m{};
    m.renderer = renderer;
    auto field = [&](std::uint64_t base, std::uint64_t offset, std::uint64_t& value) {
        constexpr auto maximum = (std::numeric_limits<std::uint64_t>::max)();
        return base <= maximum - offset && base + offset <= maximum - 7u &&
               read(base + offset, value);
    };
    if (!field(renderer, 0x60, m.owner)) return Result::Unreadable;
    if (!m.owner) return Result::NullPointer;
    if (!field(m.owner, 0xF0, m.buffer)) return Result::Unreadable;
    if (!m.buffer) return Result::NullPointer;
    if (!field(m.buffer, 0x28, m.stride) || !field(m.buffer, 0x30, m.bytes) ||
        !field(m.buffer, 0x48, m.gpuAddress)) return Result::Unreadable;
    if (m.stride != 8) return Result::BadStride;
    if (!m.bytes || m.bytes % 8) return Result::BadSize;
    m.elements = m.bytes / 8;
    if (materialID >= m.elements) return Result::MaterialOutOfRange;
    m.materialOffset = static_cast<std::uint64_t>(materialID) * 8;
    out = m;
    // GPU address is recorded only, including zero. Candidate != usable resource.
    return Result::Candidate;
}
} // namespace control_rr_part1
