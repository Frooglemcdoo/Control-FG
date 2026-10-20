#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// Exact captured native shaders only. Diagnostics remove one contribution;
// they do not change sampling quality or ship as a visual improvement.
namespace native_lighting {
struct InstructionEdit { size_t offset; const uint32_t* expected; const uint32_t* replacement; size_t words; };
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
    const InstructionEdit* edits = nullptr;
    size_t editCount = 0;
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

inline constexpr uint32_t kShadowBypass710Expected[] = {0x800002du,0x100082u,0x3u,0x100e46u,0x9u,0x207396u,0x2u,0x2u};
inline constexpr uint32_t kShadowBypass710Replacement[] = {0x5000036u,0x100082u,0x3u,0x4001u,0x3f800000u,0x100003au,0x100003au,0x100003au};
inline constexpr uint32_t kShadowBypass4429Expected[] = {0x800002du,0x100032u,0x9u,0x100e46u,0x9u,0x2078d6u,0x2u,0x2u};
inline constexpr uint32_t kShadowBypass4429Replacement[] = {0x5000036u,0x100032u,0x9u,0x4001u,0x3f800000u,0x100003au,0x100003au,0x100003au};
inline constexpr uint32_t kShadowBypass7268Expected[] = {0x9000037u,0x100082u,0x1cu,0x10003au,0x1eu,0x10003au,0x1fu,0x10003au,0x1cu};
inline constexpr uint32_t kShadowBypass7268Replacement[] = {0x5000036u,0x100082u,0x1cu,0x4001u,0x3f800000u,0x100003au,0x100003au,0x100003au,0x100003au};
inline constexpr uint32_t kShadowBypass11856Expected[] = {0x9000037u,0x100082u,0x11u,0x10003au,0x13u,0x4001u,0x3f800000u,0x10003au,0x11u};
inline constexpr uint32_t kShadowBypass11856Replacement[] = {0x5000036u,0x100082u,0x11u,0x4001u,0x3f800000u,0x100003au,0x100003au,0x100003au,0x100003au};
inline constexpr InstructionEdit kShadowBypassEdits[] = {{13992, kShadowBypass710Expected, kShadowBypass710Replacement, 8},{28868, kShadowBypass4429Expected, kShadowBypass4429Replacement, 8},{40224, kShadowBypass7268Expected, kShadowBypass7268Replacement, 9},{58576, kShadowBypass11856Expected, kShadowBypass11856Replacement, 9}};
inline constexpr unsigned char kShadowBypassChecksum[] = {6,209,151,206,182,92,103,174,25,129,232,225,40,64,162,220};
inline constexpr ShaderPatch kShadowBypass = {
    "ShadowBypass", kDirect.size, kDirect.nativeSha, "c6661e9f3b9fe0bb297eb55d2a069df029b09bd93a2faa67a4b09ed3c986133a",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, kShadowBypassChecksum,
    kShadowBypassEdits, 4
};
inline constexpr uint32_t kPatternOff653Expected[] = {0xd000048u,0x100082u,0x3u,0x100046u,0xcu,0x207396u,0x11u,0x11u,0x206000u,0x2u,0x8u,0x4001u,0x0u};
inline constexpr uint32_t kPatternOff653Replacement[] = {0x5000036u,0x100082u,0x3u,0x4001u,0x3f800000u,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au};
inline constexpr uint32_t kPatternOff7525Expected[] = {0xd000048u,0x100072u,0x20u,0x100086u,0x1fu,0x207e46u,0xeu,0xeu,0x206000u,0x3u,0xbu,0x4001u,0x0u};
inline constexpr uint32_t kPatternOff7525Replacement[] = {0x5000036u,0x100072u,0x20u,0x4001u,0x3f800000u,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au,0x100003au};
inline constexpr InstructionEdit kPatternOffEdits[] = {{13764, kPatternOff653Expected, kPatternOff653Replacement, 13},{41252, kPatternOff7525Expected, kPatternOff7525Replacement, 13}};
inline constexpr unsigned char kPatternOffChecksum[] = {100,186,153,71,113,24,220,164,106,117,234,102,55,175,186,254};
inline constexpr ShaderPatch kPatternOff = {
    "PatternOff", kDirect.size, kDirect.nativeSha, "07a28cb8848d31c54cb08d73c333ec517a89117dd923e8c14ff546597b9bd2c7",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, kPatternOffChecksum,
    kPatternOffEdits, 2
};

// Individual shadow paths reuse the validated all-shadow instruction edits.
inline constexpr InstructionEdit kScreenShadowsEdits[] = {{13992, kShadowBypass710Expected, kShadowBypass710Replacement, 8}, {28868, kShadowBypass4429Expected, kShadowBypass4429Replacement, 8}};
inline constexpr unsigned char kScreenShadowsChecksum[] = {55,29,56,184,20,157,103,56,138,55,67,224,232,188,94,255};
inline constexpr ShaderPatch kScreenShadows = {
    "ScreenShadows", kDirect.size, kDirect.nativeSha, "d98cafbd21bb31173fca18fbdb5c285ea67ea660a42e92caa20afa438b2f58fe",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, kScreenShadowsChecksum,
    kScreenShadowsEdits, 2
};
inline constexpr InstructionEdit kSpotShadowsEdits[] = {{40224, kShadowBypass7268Expected, kShadowBypass7268Replacement, 9}};
inline constexpr unsigned char kSpotShadowsChecksum[] = {234,118,87,245,127,212,205,255,76,56,47,209,20,202,182,180};
inline constexpr ShaderPatch kSpotShadows = {
    "SpotShadows", kDirect.size, kDirect.nativeSha, "6cdd55f4b98012cd7bfd1418e2e48339e092e11e2b465493bb4103fd0ade4b9d",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, kSpotShadowsChecksum,
    kSpotShadowsEdits, 1
};
inline constexpr InstructionEdit kPointShadowsEdits[] = {{58576, kShadowBypass11856Expected, kShadowBypass11856Replacement, 9}};
inline constexpr unsigned char kPointShadowsChecksum[] = {144,148,106,187,41,30,131,229,235,44,43,92,143,83,132,26};
inline constexpr ShaderPatch kPointShadows = {
    "PointShadows", kDirect.size, kDirect.nativeSha, "ecb18587d898d3150dd9709b5f3716fa177a645e65b89a579d30e62e95000020",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, kPointShadowsChecksum,
    kPointShadowsEdits, 1
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
    for (size_t i = 0; i < patch.editCount; ++i) {
        const auto& edit = patch.edits[i];
        if (edit.offset > size || edit.words > (size - edit.offset) / 4 ||
            std::memcmp(source + edit.offset, edit.expected, edit.words * 4)) return false;
    }
    result.assign(source, source + size);
    for (size_t i = 0; i < patch.editCount; ++i) {
        const auto& edit = patch.edits[i];
        std::memcpy(result.data() + edit.offset, edit.replacement, edit.words * 4);
    }
    const uint32_t zero = 0;
    for (size_t i = 0; i < patch.zeroCount; ++i)
        std::memcpy(result.data() + patch.instructionOffset + patch.zeroWords[i] * 4, &zero, 4);
    std::memcpy(result.data() + 4, patch.checksum, 16);
    return true;
}
} // namespace native_lighting
