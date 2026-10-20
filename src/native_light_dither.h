#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// R51 diagnostic: change only the two output-dither amplitudes in the exact
// non-RT deferred-light shader captured on Steam. All sampling stays native.
namespace control_lighting {
inline constexpr size_t kDeferredShaderBytes = 69704;
inline constexpr char kDeferredShaderSha256[] =
    "bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff";
inline constexpr char kNoDitherSha256[] =
    "f451686b9d84e3fadfa95faab1d276115c08469cf0d11b601432e3eb7caa19c7";
inline constexpr size_t kDitherInstructionOffset = 69372;
inline constexpr uint32_t kDitherInstruction[] = {
    251658290, 1048626, 0, 1048582, 0, 16386,
    1017370378, 1036831949, 0, 0, 16386, 1065353216, 1065353216, 0, 0
};
inline constexpr unsigned char kNoDitherChecksum[] = {
    88,121,92,5,29,66,112,1,130,136,224,79,212,32,140,122
};

// Caller supplies a freshly computed SHA-256. Both the full identity and the
// complete instruction must match before copying or modifying anything.
inline bool MakeNoDitherShader(const void* bytes, size_t size, const char* sha,
                              std::vector<unsigned char>& output) {
    output.clear();
    if (!bytes || !sha || size != kDeferredShaderBytes ||
        std::strcmp(sha, kDeferredShaderSha256) != 0) return false;
    const auto* source = static_cast<const unsigned char*>(bytes);
    if (std::memcmp(source + kDitherInstructionOffset, kDitherInstruction,
                    sizeof(kDitherInstruction)) != 0) return false;
    output.assign(source, source + size);
    // mad r0.xy, centered_noise, (0.02, 0.10), (1, 1) -> amplitudes (0, 0).
    std::memset(output.data() + kDitherInstructionOffset + 6 * sizeof(uint32_t),
                0, 2 * sizeof(uint32_t));
    // Precomputed for this exact patch; independently verified against all 509
    // captured DXBC checksums. Runtime also verifies the full patched SHA-256.
    std::memcpy(output.data() + 4, kNoDitherChecksum, sizeof(kNoDitherChecksum));
    return true;
}
}
