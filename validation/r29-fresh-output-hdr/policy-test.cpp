#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_hdr_transition_policy.h"

int main() {
    using namespace control_fg_hdr_transition;
    static_assert(kFreshFramesRequired == 2, "r29 retains the two-fresh-frame post-resize warmup");

    assert(!DisplayDomainFlip(false, false, true));
    assert(!DisplayDomainFlip(false, true, false));
    assert(!DisplayDomainFlip(true, false, false));
    assert(!DisplayDomainFlip(true, true, true));
    assert(DisplayDomainFlip(true, false, true));
    assert(DisplayDomainFlip(true, true, false));

    assert(HoldForPreResizeDisplayTransition(true, 7, 7));
    assert(!HoldForPreResizeDisplayTransition(false, 7, 7));
    assert(!HoldForPreResizeDisplayTransition(true, 8, 7));
    assert(!BridgeTransitionObserved(true, 7, 7));
    assert(BridgeTransitionObserved(true, 8, 7));
    assert(!BridgeTransitionObserved(false, 8, 7));

    assert(!NeedsCommittedOffPresent(false));
    assert(NeedsCommittedOffPresent(true));
    assert(QuiesceCommitted(false, false, false));
    assert(QuiesceCommitted(true, true, true));

    for (std::uint64_t g = 1; g < 10000; ++g) {
        std::uint32_t c = 0;
        c = NextFreshFrameCount(true, g, ~0ull, 100, 0, c);
        assert(c == 1);
        c = NextFreshFrameCount(true, g, g, 101, 100, c);
        assert(c == 2);
        assert(WarmupComplete(c));
        assert(CanEnable(true, g, g));
    }

    std::puts("PASS r29 fresh-output HDR guard policy: pre-resize domain gate, bridge handoff, resize fallback, and two-fresh-frame warmup retained");
    return 0;
}
