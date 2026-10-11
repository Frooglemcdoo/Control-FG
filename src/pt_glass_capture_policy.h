#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace PTGlassCapture {
constexpr std::size_t kMaximumBytes = 2u * 1024u * 1024u;
inline std::uint32_t Word(const unsigned char* p) noexcept {
    std::uint32_t v; std::memcpy(&v, p, sizeof(v)); return v;
}
// Only shader libraries exporting the observed glass ray-generation entry
// qualify. Search RDAT, never arbitrary strings in a pixel/compute shader.
inline bool IsGlassLibrary(const unsigned char* bytes, std::size_t size) noexcept {
    if (!bytes || size < 32 || size > kMaximumBytes ||
        std::memcmp(bytes, "DXBC", 4) || Word(bytes + 24) != size) return false;
    const auto count = Word(bytes + 28);
    if (!count || count > (size - 32) / 4) return false;
    bool library = false, glass = false;
    const char entry[] = "transparentRayGeneration";
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto offset = Word(bytes + 32 + i * 4);
        if (offset < 32 + count * 4 || offset > size - 8) return false;
        const auto length = Word(bytes + offset + 4);
        if (length > size - offset - 8) return false;
        const auto* data = bytes + offset + 8;
        if (!std::memcmp(bytes + offset, "DXIL", 4) && length >= 16)
            library = (Word(data) >> 16) == 6u;
        if (!std::memcmp(bytes + offset, "RDAT", 4)) {
            for (std::size_t j = 0; j + sizeof(entry) - 1 <= length; ++j)
                if (!std::memcmp(data + j, entry, sizeof(entry) - 1)) glass = true;
        }
    }
    return library && glass;
}
}
