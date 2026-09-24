#include "patches.h"
#include "log.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace patches
{
namespace
{
constexpr std::array<uint8_t, 13> kNgxPattern{
    0x84, 0xD2,                               // test dl, dl
    0x0F, 0x84, 0x03, 0x01, 0x00, 0x00,       // je   rel32
    0xBE, 0x05, 0x00, 0x00, 0x00              // mov  esi, 5
};
constexpr size_t kNgxPatchOffset = 2;
constexpr std::array<uint8_t, 6> kNgxOriginal{
    0x0F, 0x84, 0x03, 0x01, 0x00, 0x00 };
constexpr std::array<uint8_t, 6> kNgxReplacement{
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

constexpr size_t kWrapperPatternSize = 10;
constexpr size_t kWrapperMaximumOffset = 1;
constexpr size_t kWrapperPatchOffset = 7;
constexpr std::array<uint8_t, 3> kWrapperOriginal{ 0x0F, 0x42, 0xD1 };
constexpr std::array<uint8_t, 3> kWrapperReplacement{ 0x90, 0x90, 0x90 };

constexpr bool IsSupportedMaximum(uint32_t maximum) noexcept
{
    return maximum == 1 || maximum == 3 || maximum == 5;
}

const IMAGE_NT_HEADERS64* ImageHeaders(HMODULE module) noexcept
{
    const auto* base = reinterpret_cast<const uint8_t*>(module);
    if (!base)
        return nullptr;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0
        || static_cast<size_t>(dos->e_lfanew) > 1024 * 1024)
        return nullptr;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE
        || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return nullptr;
    return nt;
}

// Applies `replacement` over `address`, restoring the original page
// protection afterwards. Already-patched bytes are reported as success.
bool ApplyBytes(const wchar_t* label, const uint8_t* base, uint8_t* address,
    const uint8_t* original, const uint8_t* replacement, size_t size,
    const wchar_t* path) noexcept
{
    const size_t rva = static_cast<size_t>(address - base);
    if (std::memcmp(address, replacement, size) == 0)
    {
        mfglog::Write(L"%s: already patched at RVA 0x%zX: %s",
            label, rva, path);
        return true;
    }
    if (std::memcmp(address, original, size) != 0)
    {
        mfglog::Write(L"%s: matched context but original bytes differ: %s",
            label, path);
        return false;
    }

    DWORD oldProtection = 0;
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        mfglog::Write(L"%s: VirtualProtect failed (%lu): %s",
            label, GetLastError(), path);
        return false;
    }
    std::memcpy(address, replacement, size);
    FlushInstructionCache(GetCurrentProcess(), address, size);
    DWORD ignored = 0;
    if (!VirtualProtect(address, size, oldProtection, &ignored))
    {
        mfglog::Write(L"%s: protection restore failed (%lu): %s",
            label, GetLastError(), path);
        return false;
    }
    mfglog::Write(L"%s: patched RVA 0x%zX: %s", label, rva, path);
    return true;
}

// Calls `visit(candidate, remaining)` at every offset of every executable
// section. The visitor returns true when the bytes match.
template <typename Visitor>
uint8_t* FindUniqueInCode(HMODULE module, size_t patternSize,
    Visitor&& visit, size_t& matchCount) noexcept
{
    matchCount = 0;
    const auto* base = reinterpret_cast<const uint8_t*>(module);
    const auto* nt = ImageHeaders(module);
    if (!nt)
        return nullptr;

    uint8_t* match = nullptr;
    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned index = 0; index < nt->FileHeader.NumberOfSections;
         ++index, ++section)
    {
        if ((section->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0
            || section->VirtualAddress >= nt->OptionalHeader.SizeOfImage)
            continue;
        auto* begin = const_cast<uint8_t*>(base + section->VirtualAddress);
        const size_t available =
            nt->OptionalHeader.SizeOfImage - section->VirtualAddress;
        const size_t size = std::min<size_t>(available,
            std::max<size_t>(section->Misc.VirtualSize,
                section->SizeOfRawData));
        if (size < patternSize)
            continue;
        for (size_t offset = 0; offset + patternSize <= size; ++offset)
        {
            if (!visit(begin + offset, size - offset))
                continue;
            match = begin + offset;
            ++matchCount;
        }
    }
    return match;
}
}

Result PatchNgxDeviceSupport(HMODULE module, const wchar_t* path) noexcept
{
    constexpr const wchar_t* kLabel = L"NGX device support";
    const auto* base = reinterpret_cast<const uint8_t*>(module);
    size_t matchCount = 0;
    uint8_t* const match = FindUniqueInCode(module, kNgxPattern.size(),
        [](const uint8_t* candidate, size_t) noexcept {
            // The conditional jump itself may already be NOPs, so compare the
            // surrounding context and accept either form in between.
            return std::memcmp(candidate, kNgxPattern.data(),
                       kNgxPatchOffset) == 0
                && std::memcmp(candidate + kNgxPatchOffset + kNgxOriginal.size(),
                       kNgxPattern.data() + kNgxPatchOffset + kNgxOriginal.size(),
                       kNgxPattern.size() - kNgxPatchOffset - kNgxOriginal.size())
                    == 0
                && (std::memcmp(candidate + kNgxPatchOffset,
                        kNgxOriginal.data(), kNgxOriginal.size()) == 0
                    || std::memcmp(candidate + kNgxPatchOffset,
                        kNgxReplacement.data(), kNgxReplacement.size()) == 0);
        }, matchCount);

    if (matchCount == 0)
        return {};
    if (matchCount != 1 || !match)
    {
        mfglog::Write(L"%s: expected one code pattern, found %zu: %s",
            kLabel, matchCount, path);
        return {true, false};
    }
    return {true, ApplyBytes(kLabel, base, match + kNgxPatchOffset,
        kNgxOriginal.data(), kNgxReplacement.data(), kNgxOriginal.size(),
        path)};
}

Result PatchStreamlineMaximum(HMODULE module, const wchar_t* path) noexcept
{
    constexpr const wchar_t* kLabel = L"Streamline maximum";
    const auto* base = reinterpret_cast<const uint8_t*>(module);
    size_t matchCount = 0;
    uint8_t* const match = FindUniqueInCode(module, kWrapperPatternSize,
        [](const uint8_t* candidate, size_t) noexcept {
            if (candidate[0] != 0xBA) // mov edx, imm32
                return false;
            uint32_t maximum = 0;
            std::memcpy(&maximum, candidate + kWrapperMaximumOffset,
                sizeof(maximum));
            return IsSupportedMaximum(maximum)
                && candidate[5] == 0x3B && candidate[6] == 0xCA // cmp ecx, edx
                && ((candidate[7] == 0x0F && candidate[8] == 0x42
                        && candidate[9] == 0xD1) // cmovb edx, ecx
                    || (candidate[7] == 0x90 && candidate[8] == 0x90
                        && candidate[9] == 0x90));
        }, matchCount);

    if (matchCount == 0)
        return {};
    if (matchCount != 1 || !match)
    {
        mfglog::Write(L"%s: expected one 1/3/5 profile, found %zu: %s",
            kLabel, matchCount, path);
        return {true, false};
    }

    uint32_t compiledMaximum = 0;
    std::memcpy(&compiledMaximum, match + kWrapperMaximumOffset,
        sizeof(compiledMaximum));
    const bool patched = ApplyBytes(kLabel, base,
        match + kWrapperPatchOffset, kWrapperOriginal.data(),
        kWrapperReplacement.data(), kWrapperReplacement.size(), path);
    mfglog::Write(L"%s: compiled maximum %u generated frames (native %ux): %s",
        kLabel, compiledMaximum, SafeMaximumMultiplier(compiledMaximum), path);
    return {true, patched, compiledMaximum};
}
}
