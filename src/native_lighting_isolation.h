#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// Exact captured native shaders only. Diagnostics remove one contribution;
// they do not change sampling quality or ship as a visual improvement.
namespace native_lighting {
struct ShaderPatch {
    const char* name;
    size_t size;
    const char* nativeSha;
    const char* patchedSha;
    size_t instructionOffset;
    const uint32_t* instruction;
    size_t instructionWords;
    const size_t* zeroWords;
    size_t zeroCount;
    const unsigned char* checksum;
};
inline constexpr uint32_t kDirectInstruction[] = {0xf000032u,0x100032u,0x0u,0x100006u,0x0u,0x4002u,0x3ca3d70au,0x3dcccccdu,0x0u,0x0u,0x4002u,0x3f800000u,0x3f800000u,0x0u,0x0u};
inline constexpr size_t kDirectZeroWords[] = {6,7,11,12};
inline constexpr unsigned char kDirectChecksum[] = {23,51,110,60,43,117,249,45,23,77,248,47,237,225,14,67};
inline constexpr ShaderPatch kDirect = {
    "Direct", 69704,
    "bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff",
    "46fd09e5d4e1612e601086874af30513b8e0f9641a6ba95a9791706406c58f16",
    69372, kDirectInstruction, 15, kDirectZeroWords, 4, kDirectChecksum
};
inline constexpr size_t kDirectDiffuseZeroWords[] = {6,11};
inline constexpr unsigned char kDirectDiffuseChecksum[] = {226,114,59,194,212,169,172,229,77,100,1,223,241,21,244,3};
inline constexpr ShaderPatch kDirectDiffuse = {
    "DirectDiffuse", kDirect.size, kDirect.nativeSha,
    "20c171cfec68f40055e07771082822ee6885fe4fdb13bbd622c9008647bb17c0",
    kDirect.instructionOffset, kDirectInstruction, 15, kDirectDiffuseZeroWords, 2, kDirectDiffuseChecksum
};
inline constexpr size_t kDirectSpecularZeroWords[] = {7,12};
inline constexpr unsigned char kDirectSpecularChecksum[] = {189,233,26,206,10,30,148,198,130,87,242,157,1,178,180,73};
inline constexpr ShaderPatch kDirectSpecular = {
    "DirectSpecular", kDirect.size, kDirect.nativeSha,
    "230f46a74e01806049f36fb8ebe7135b82c508668aa8af4b0759167a39de39cc",
    kDirect.instructionOffset, kDirectInstruction, 15, kDirectSpecularZeroWords, 2, kDirectSpecularChecksum
};
inline constexpr uint32_t kBakedGiInstruction[] = {0x7000038u,0x100082u,0x0u,0x10003au,0x0u,0x4001u,0x3ea2f983u};
inline constexpr size_t kBakedGiZeroWords[] = {6};
inline constexpr unsigned char kBakedGiChecksum[] = {43,215,128,24,199,53,17,76,124,7,139,176,78,58,12,230};
inline constexpr ShaderPatch kBakedGi = {
    "BakedGi", 17040,
    "da7d72eb08c2ce1cf755ee5c24d893791768ea0aaafa434eaf0b193275bd6dcb",
    "9489ba19c28bc4f7beafec6dd9afd12b9401d2ed443a31393fdf72bddb00a914",
    16416, kBakedGiInstruction, 7, kBakedGiZeroWords, 1, kBakedGiChecksum
};

inline bool MakeDisabledContribution(const ShaderPatch& patch, const void* bytes,
                                     size_t size, const char* sha,
                                     std::vector<unsigned char>& result) {
    result.clear();
    if (!bytes || !sha || size != patch.size || std::strcmp(sha, patch.nativeSha)) return false;
    const auto* source = static_cast<const unsigned char*>(bytes);
    if (patch.instructionOffset > size || patch.instructionWords > (size - patch.instructionOffset) / 4) return false;
    if (std::memcmp(source + patch.instructionOffset, patch.instruction, patch.instructionWords * 4)) return false;
    for (size_t i = 0; i < patch.zeroCount; ++i)
        if (patch.zeroWords[i] >= patch.instructionWords) return false;
    result.assign(source, source + size);
    const uint32_t zero = 0;
    for (size_t i = 0; i < patch.zeroCount; ++i)
        std::memcpy(result.data() + patch.instructionOffset + patch.zeroWords[i] * 4, &zero, 4);
    std::memcpy(result.data() + 4, patch.checksum, 16);
    return true;
}
} // namespace native_lighting
