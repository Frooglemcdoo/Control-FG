#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace rt_spot_capture {
using ShaderIdentifier = std::array<unsigned char, 32>;
struct IdentifierReplacement {
    ShaderIdentifier native;
    ShaderIdentifier capture;
};
// Reference implementation for validating a captured table. Not called on the
// render thread. The eventual GPU-copy adapter must satisfy the same contract.
// A shader identifier belongs to a state object. Local root arguments belong
// to each record and MUST survive replacement byte-for-byte.
inline bool TranslateHitTable(const std::vector<unsigned char>& source,
                              const std::vector<IdentifierReplacement>& replacements,
                              std::vector<unsigned char>& destination) {
    constexpr std::size_t stride = 96; // R117 verified native spotlight layout.
    constexpr std::size_t maximumBytes = 8 * 1024 * 1024;
    if (source.empty() || source.size() > maximumBytes || source.size() % stride ||
        replacements.empty() || replacements.size() > 16) return false;
    const ShaderIdentifier empty{};
    for (std::size_t i = 0; i < replacements.size(); ++i) {
        if (replacements[i].native == empty || replacements[i].capture == empty) return false;
        for (std::size_t j = 0; j < i; ++j)
            if (replacements[i].native == replacements[j].native) return false;
    }
    // Validate all records before publishing anything. Unknown records are an
    // unsupported layout, never permission to guess which hit shader to use.
    std::vector<std::size_t> selected;
    selected.reserve(source.size() / stride);
    for (std::size_t at = 0; at < source.size(); at += stride) {
        std::size_t found = replacements.size();
        for (std::size_t i = 0; i < replacements.size(); ++i)
            if (!std::memcmp(source.data() + at, replacements[i].native.data(), 32)) {
                found = i;
                break;
            }
        if (found == replacements.size()) return false;
        selected.push_back(found);
    }
    auto translated = source;
    for (std::size_t i = 0; i < selected.size(); ++i)
        std::memcpy(translated.data() + i * stride, replacements[selected[i]].capture.data(), 32);
    destination.swap(translated);
    return true;
}
} // namespace rt_spot_capture
