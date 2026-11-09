#pragma once

#include <windows.h>

#include <cstdint>

// The two in-memory code patches that let an Ada GPU reach NVIDIA's
// multi-frame path. Both are strictly fail-closed: a pattern that appears
// zero times or more than once is never touched.
namespace patches
{
struct Result
{
    bool candidate = false; // the module contained the expected code shape
    bool patched = false;
    uint32_t compiledMaximum = 0; // Streamline clamp only: 1, 3 or 5
};

// Turns the NGX per-GPU support branch in nvngx_dlssg.dll into a fallthrough.
//
//   test dl, dl
//   je   reject        <- six bytes replaced with NOPs
//   mov  esi, 5
Result PatchNgxDeviceSupport(HMODULE module, const wchar_t* path) noexcept;

// Removes the Streamline wrapper's clamp on the requested generated-frame
// count in sl.dlss_g.dll.
//
//   mov   edx, <compiled maximum>   ; 1, 3 or 5
//   cmp   ecx, edx
//   cmovb edx, ecx                  <- three bytes replaced with NOPs
Result PatchStreamlineMaximum(HMODULE module, const wchar_t* path) noexcept;

// Rewrites every NVAPI arch-id comparison against 0x1b0 (GB20x, Blackwell) to
// 0x190 (AD10x, Ada) in the DLSS-G provider.
//
// This is what actually raises the advertised frame count. The provider gates
// multi-frame on the arch id in more than one place, and they do different
// jobs: one decides what DLSSG.MultiFrameCountMax advertises, another feeds a
// runtime capability flag that drives generation itself. Patching only the
// first makes 3x/4x appear, be accepted, and render black -- so all of them
// are rewritten together or none are.
//
//   3D B0 01 00 00           cmp eax, 0x1b0
//   81 /7 B0 01 00 00        cmp r32, 0x1b0
//
// A `mov r32, 0x1b0` is the arch-id lookup table returning Blackwell's own id,
// not a gate, and is deliberately left alone.
struct ArchGateResult
{
    size_t found = 0;
    size_t patched = 0;
};
ArchGateResult PatchArchGates(HMODULE module, const wchar_t* path) noexcept;

// Forces the DLSS-G Streamline plugin onto its software (RSYNC) pacing path.
//
// Blackwell paces multi-frame output with hardware flip metering; Ada has no
// such hardware, so with it left on the present queue waits for something that
// never happens -- 3x and above freeze the image while audio keeps running.
// Streamline already ships the fallback; this only has to select it.
//
// Neither the field's offset nor its polarity can be hardcoded: they differ
// between plugin builds. Both are derived from the plugin's own fallback code,
// found via the marker string it logs.
struct FlipMeterResult
{
    bool located = false;   // this module is the DLSS-G plugin
    bool derived = false;   // the wanted (offset, value) was read successfully
    uint32_t offset = 0;
    uint32_t value = 0;
    size_t sites = 0;
};
FlipMeterResult PatchFlipMetering(HMODULE module, const wchar_t* path) noexcept;

// A wrapper compiled for N generated frames natively supports an N+1
// multiplier; anything unrecognized falls back to plain 2x.
constexpr uint32_t SafeMaximumMultiplier(uint32_t compiledMaximum) noexcept
{
    return (compiledMaximum == 1 || compiledMaximum == 3
        || compiledMaximum == 5) ? compiledMaximum + 1u : 2u;
}
}
