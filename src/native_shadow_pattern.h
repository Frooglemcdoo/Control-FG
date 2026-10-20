#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// R52 diagnostic: fix the spotlight PCF sampling basis in the exact
// non-RT deferred-light shader captured on Steam. Tap count and weights stay native.
namespace control_lighting {
inline constexpr size_t kDeferredShaderBytes = 69704;
inline constexpr char kDeferredShaderSha256[] =
    "bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff";
inline constexpr char kFixedPatternSha256[] =
    "ce87eef29ac053359c9c4ac5ce916174120169c36c07aa1dddddf05eea553eb6";
inline constexpr size_t kShadowPatternInstructionOffset = 12720;
inline constexpr uint32_t kShadowPatternInstruction[] = {
    251658290,1048818,4,1052230,4,16386,1073741824,1073741824,1073741824,1073741824,16386,3212836864,3212836864,3212836864,3212836864
};
inline constexpr unsigned char kFixedPatternChecksum[] = {
    154,36,74,251,144,70,132,30,74,130,53,208,1,161,51,202
};

// Caller supplies a freshly computed SHA-256. Both the full identity and the
// complete instruction must match before copying or modifying anything.
inline bool MakeFixedPatternShader(const void* bytes, size_t size, const char* sha,
                              std::vector<unsigned char>& output) {
    output.clear();
    if (!bytes || !sha || size != kDeferredShaderBytes ||
        std::strcmp(sha, kDeferredShaderSha256) != 0) return false;
    const auto* source = static_cast<const unsigned char*>(bytes);
    if (std::memcmp(source + kShadowPatternInstructionOffset, kShadowPatternInstruction,
                    sizeof(kShadowPatternInstruction)) != 0) return false;
    output.assign(source, source + size);
    // Replace the random 2D sampling basis with fixed unit X/Y axes.
    // The PCF taps and weights remain intact; output dither remains native.
    const uint32_t zeroScale[4]{};
    const uint32_t fixedBasis[4]{0x3f800000u,0,0,0x3f800000u};
    std::memcpy(output.data()+kShadowPatternInstructionOffset+24,zeroScale,sizeof(zeroScale));
    std::memcpy(output.data()+kShadowPatternInstructionOffset+44,fixedBasis,sizeof(fixedBasis));
    // Precomputed for this exact patch; independently verified against all 509
    // captured DXBC checksums. Runtime also verifies the full patched SHA-256.
    std::memcpy(output.data() + 4, kFixedPatternChecksum, sizeof(kFixedPatternChecksum));
    return true;
}
}
