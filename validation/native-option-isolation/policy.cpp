#include "../../src/rr_native_option_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace control_rr;
    assert(!NativeRenderOption(false, false, false));
    assert(!NativeRenderOption(true, false, false));
    assert(NativeRenderOption(true, true, false));
    assert(!NativeRenderOption(true, true, true));

    constexpr std::uintptr_t site = 0x18011de2eull;
    constexpr std::uintptr_t nativeOption = 0x180914620ull;
    const RipByteCompare6 original{0x38,0x1d,0xec,0x67,0x7f,0x00};
    assert(DecodeRipByteCompareTarget(original, site) == nativeOption);

    RipByteCompare6 replacement{};
    constexpr std::uintptr_t mirror = 0x180200000ull;
    assert(RedirectRipByteCompare(original, site, nativeOption, mirror, replacement));
    assert(DecodeRipByteCompareTarget(replacement, site) == mirror);
    assert(replacement[0] == 0x38 && replacement[1] == 0x1d);

    RipByteCompare6 rejected{};
    assert(!RedirectRipByteCompare(original, site, nativeOption + 1, mirror, rejected));
    auto wrongOpcode = original; wrongOpcode[0] = 0x39;
    assert(!RedirectRipByteCompare(wrongOpcode, site, nativeOption, mirror, rejected));
    assert(!RedirectRipByteCompare(original, site, nativeOption, 0x7fff00000000ull, rejected));

    puts("PASS native RR option isolation policy and exact jitter RIP-byte redirect encoding");
}
