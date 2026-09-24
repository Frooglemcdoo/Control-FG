// The two capability gates that decide whether Ada is allowed to generate more
// than one frame, and whether those frames ever reach the screen.
//
// Neither is a clamp on the request. The provider decides what to advertise
// from the NVAPI architecture id, and the plugin picks its pacing path from a
// flag whose offset and polarity move between builds. Both are derived from
// the loaded image rather than hardcoded.
//
// Everything here edits the mapped image only. NGX verifies the provider's
// Authenticode signature when it loads the file, so the same bytes changed on
// disk make frame generation disappear entirely; the mapped copy is never
// re-checked.

#include "patches.h"
#include "log.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace patches
{
namespace
{
const IMAGE_NT_HEADERS64* Headers(HMODULE module) noexcept
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

// Calls visit(start, size) once per section carrying `characteristic`.
template <typename Visitor>
void ForEachSection(HMODULE module, DWORD characteristic,
    Visitor&& visit) noexcept
{
    const auto* base = reinterpret_cast<const uint8_t*>(module);
    const auto* nt = Headers(module);
    if (!nt)
        return;
    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned index = 0; index < nt->FileHeader.NumberOfSections;
         ++index, ++section)
    {
        if ((section->Characteristics & characteristic) == 0
            || section->VirtualAddress >= nt->OptionalHeader.SizeOfImage)
            continue;
        const size_t available =
            nt->OptionalHeader.SizeOfImage - section->VirtualAddress;
        const size_t size = std::min<size_t>(available,
            std::max<size_t>(section->Misc.VirtualSize,
                section->SizeOfRawData));
        visit(const_cast<uint8_t*>(base + section->VirtualAddress), size);
    }
}

bool WriteCode(uint8_t* at, const uint8_t* bytes, size_t size) noexcept
{
    DWORD oldProtection = 0;
    if (!VirtualProtect(at, size, PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;
    std::memcpy(at, bytes, size);
    DWORD ignored = 0;
    VirtualProtect(at, size, oldProtection, &ignored);
    FlushInstructionCache(GetCurrentProcess(), at, size);
    return true;
}

constexpr uint8_t kArchBlackwell = 0xB0; // low octet of 0x1b0, GB20x
constexpr uint8_t kArchAda = 0x90;       // low octet of 0x190, AD10x
}

ArchGateResult PatchArchGates(HMODULE module, const wchar_t* path) noexcept
{
    constexpr const wchar_t* kLabel = L"Arch gate";
    std::vector<uint8_t*> sites;
    ForEachSection(module, IMAGE_SCN_MEM_EXECUTE,
        [&](uint8_t* start, size_t size) noexcept {
            if (size < 6)
                return;
            for (size_t offset = 0; offset + 6 <= size; ++offset)
            {
                // 3D id32 -- cmp eax, imm32
                if (start[offset] == 0x3D
                    && start[offset + 1] == kArchBlackwell
                    && start[offset + 2] == 0x01
                    && start[offset + 3] == 0x00
                    && start[offset + 4] == 0x00)
                {
                    sites.push_back(start + offset + 1);
                    continue;
                }
                // 81 /7 id32 -- cmp r32, imm32
                if (start[offset] == 0x81 && start[offset + 1] >= 0xF8
                    && start[offset + 1] <= 0xFF
                    && start[offset + 2] == kArchBlackwell
                    && start[offset + 3] == 0x01
                    && start[offset + 4] == 0x00
                    && start[offset + 5] == 0x00)
                {
                    sites.push_back(start + offset + 2);
                }
            }
        });

    ArchGateResult result{};
    result.found = sites.size();
    // None means this provider does not gate the way we understand it. Many
    // means the pattern is not selective enough here. Rewriting architecture
    // comparisons on a guess is not worth it either way.
    if (sites.empty() || sites.size() > 4)
    {
        mfglog::Write(L"%s: found %zu arch-id comparisons (expected 1-4); "
            L"leaving this provider alone: %s", kLabel, sites.size(), path);
        return result;
    }

    for (uint8_t* site : sites)
    {
        if (*site == kArchAda)
        {
            ++result.patched; // already rewritten
            continue;
        }
        if (WriteCode(site, &kArchAda, 1))
            ++result.patched;
    }

    if (result.patched == 0)
    {
        mfglog::Write(L"%s: found %zu comparisons but none could be made "
            L"writable: %s", kLabel, result.found, path);
        return result;
    }
    mfglog::Write(L"%s: rewrote %zu of %zu arch-id comparisons "
        L"(0x1b0 -> 0x190): %s", kLabel, result.patched, result.found, path);
    return result;
}

namespace
{
constexpr char kFlipMarker[] = "FG1 DLL has been detected";

// The marker identifies the DLSS-G plugin regardless of what the OTA layer
// decided to call the file.
const uint8_t* FindFlipMarker(HMODULE module) noexcept
{
    constexpr size_t markerSize = sizeof(kFlipMarker) - 1;
    const uint8_t* found = nullptr;
    ForEachSection(module, IMAGE_SCN_MEM_READ,
        [&](uint8_t* start, size_t size) noexcept {
            if (found || size < markerSize)
                return;
            for (size_t offset = 0; offset + markerSize <= size; ++offset)
            {
                if (std::memcmp(start + offset, kFlipMarker, markerSize) == 0)
                {
                    found = start + offset;
                    return;
                }
            }
        });
    return found;
}

// Reads the (offset, value) the plugin's own fallback writes. That pair is the
// wanted state, whatever its polarity happens to be in this build.
bool DeriveFlipState(HMODULE module, const uint8_t* marker,
    uint32_t& wantOffset, uint32_t& wantValue) noexcept
{
    bool derived = false;
    ForEachSection(module, IMAGE_SCN_MEM_EXECUTE,
        [&](uint8_t* start, size_t size) noexcept {
            if (derived || size < 8)
                return;
            for (size_t offset = 0; offset + 8 <= size && !derived; ++offset)
            {
                // lea reg, [rip+disp32] pointing at the marker string
                if (start[offset] != 0x48 && start[offset] != 0x4C)
                    continue;
                if (start[offset + 1] != 0x8D
                    || (start[offset + 2] & 0xC7) != 0x05)
                    continue;
                int32_t displacement = 0;
                std::memcpy(&displacement, start + offset + 3,
                    sizeof(displacement));
                if (start + offset + 7 + displacement != marker)
                    continue;

                // The store follows shortly after the log call.
                constexpr size_t kWindow = 0x200;
                const size_t limit = std::min<size_t>(offset + kWindow, size);
                for (size_t scan = offset; scan + 7 <= limit; ++scan)
                {
                    // C6 /0 disp32 imm8 -- mov byte ptr [reg+disp32], imm8
                    if (start[scan] != 0xC6 || start[scan + 1] < 0x80
                        || start[scan + 1] > 0xBF)
                        continue;
                    uint32_t field = 0;
                    std::memcpy(&field, start + scan + 2, sizeof(field));
                    const uint8_t immediate = start[scan + 6];
                    if (field <= 0x100 || field >= 0x20000 || immediate > 1)
                        continue;
                    wantOffset = field;
                    wantValue = immediate;
                    derived = true;
                    break;
                }
            }
        });
    return derived;
}
}

FlipMeterResult PatchFlipMetering(HMODULE module, const wchar_t* path) noexcept
{
    constexpr const wchar_t* kLabel = L"Flip metering";
    FlipMeterResult result{};
    const uint8_t* marker = FindFlipMarker(module);
    if (!marker)
        return result; // not the DLSS-G plugin
    result.located = true;

    uint32_t wantOffset = 0;
    uint32_t wantValue = 0;
    if (!DeriveFlipState(module, marker, wantOffset, wantValue))
    {
        mfglog::Write(L"%s: this is the DLSS-G plugin but its fallback state "
            L"could not be read; leaving it alone: %s", kLabel, path);
        return result;
    }
    result.derived = true;
    result.offset = wantOffset;
    result.value = wantValue;

    const uint8_t opposite = static_cast<uint8_t>(1 - wantValue);
    const uint8_t wanted = static_cast<uint8_t>(wantValue);
    ForEachSection(module, IMAGE_SCN_MEM_EXECUTE,
        [&](uint8_t* start, size_t size) noexcept {
            if (size < 7)
                return;
            for (size_t offset = 0; offset + 7 <= size; ++offset)
            {
                // C6 /0 disp32 imm8 -- flip the immediate in place.
                if (start[offset] == 0xC6)
                {
                    if (start[offset + 1] < 0x80 || start[offset + 1] > 0xBF)
                        continue;
                    uint32_t field = 0;
                    std::memcpy(&field, start + offset + 2, sizeof(field));
                    if (field != wantOffset || start[offset + 6] != opposite)
                        continue;
                    if (WriteCode(start + offset + 6, &wanted, 1))
                        ++result.sites;
                    continue;
                }
                // 40 88 /r disp32 -- a runtime store with no immediate to
                // flip. Its REX prefix exists only to name a byte register,
                // which makes it exactly seven bytes: the same length as the
                // C6 form with the same base register. That equivalence is the
                // only reason it can be rewritten in place, so it is the only
                // register store handled here.
                if (start[offset] != 0x40 || start[offset + 1] != 0x88)
                    continue;
                const uint8_t modrm = start[offset + 2];
                if (modrm < 0x80 || modrm > 0xBF)
                    continue;
                const uint8_t rm = static_cast<uint8_t>(modrm & 7);
                if (rm == 4) // a SIB byte would follow
                    continue;
                uint32_t field = 0;
                std::memcpy(&field, start + offset + 3, sizeof(field));
                if (field != wantOffset)
                    continue;
                uint8_t replacement[7] = {
                    0xC6, static_cast<uint8_t>(0x80 | rm), 0, 0, 0, 0, wanted };
                std::memcpy(replacement + 2, &wantOffset, sizeof(wantOffset));
                if (WriteCode(start + offset, replacement, sizeof(replacement)))
                    ++result.sites;
            }
        });

    // Deriving the field is not the same as having changed anything.
    if (result.sites == 0)
    {
        mfglog::Write(L"%s: field +0x%X derived, but nothing writes it in a "
            L"form this can patch; nothing changed: %s",
            kLabel, wantOffset, path);
        return result;
    }
    mfglog::Write(L"%s: forced off -- field +0x%X pinned to %u at %zu site(s); "
        L"multi-frame should pace in software (RSYNC): %s",
        kLabel, wantOffset, wantValue, result.sites, path);
    return result;
}
}
