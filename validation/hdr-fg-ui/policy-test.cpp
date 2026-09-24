#include <cassert>
#include <cstdio>
#include "../../src/fg_hdr_ui_policy.h"

int main() {
    using namespace control_fg_hdr_ui;
    constexpr unsigned HUD=1u<<2;
    constexpr unsigned UI=1u<<3;
    constexpr unsigned RGB10=24u;
    constexpr unsigned FP16=10u;

    assert(!ShouldEnableRecomposition(false, HUD|UI, HUD, UI, false, RGB10, RGB10));
    assert(!ShouldEnableRecomposition(true, HUD, HUD, UI, false, RGB10, RGB10));
    assert(!ShouldEnableRecomposition(true, UI, HUD, UI, false, RGB10, RGB10));
    assert( ShouldEnableRecomposition(true, HUD|UI, HUD, UI, false, RGB10, RGB10));

    // HDR must fail closed unless HUDless is in the same RGB10 domain as the
    // intercepted HDR10 presentation backbuffer. Raw FP16/scRGB is rejected.
    assert(!HudlessDomainReady(true, FP16, RGB10));
    assert( HudlessDomainReady(true, RGB10, RGB10));
    assert(!ShouldEnableRecomposition(true, HUD|UI, HUD, UI, true, FP16, RGB10));
    assert( ShouldEnableRecomposition(true, HUD|UI, HUD, UI, true, RGB10, RGB10));

    assert(HudlessOptionFormat(true, false, RGB10, RGB10)==0u);
    assert(HudlessOptionFormat(true, true, RGB10, RGB10)==RGB10);
    assert(HudlessOptionFormat(false, true, FP16, RGB10)==FP16);

    std::puts("PASS HDR FG UI policy: HDR rejects FP16 HUDless, accepts RGB10/PQ-domain HUDless, SDR behavior retained");
    return 0;
}
