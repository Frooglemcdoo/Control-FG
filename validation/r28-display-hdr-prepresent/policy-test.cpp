#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_hdr_transition_policy.h"

int main() {
    using namespace control_fg_hdr_transition;
    static_assert(kFreshFramesRequired == 2, "r28 retains the two-fresh-frame post-resize warmup");

    // Windows HDR hotkey: no transition should arm before a baseline exists.
    assert(!DisplayDomainFlip(false, false, true));
    assert(!DisplayDomainFlip(false, true, false));
    assert(!DisplayDomainFlip(true, false, false));
    assert(!DisplayDomainFlip(true, true, true));
    assert(DisplayDomainFlip(true, false, true));
    assert(DisplayDomainFlip(true, true, false));

    // Once the output domain flips, FG must remain off until the bridge itself
    // publishes a new HDR/SDR swap-chain generation.
    assert(HoldForPreResizeDisplayTransition(true, 7, 7));
    assert(!HoldForPreResizeDisplayTransition(false, 7, 7));
    assert(!HoldForPreResizeDisplayTransition(true, 8, 7));
    assert(!BridgeTransitionObserved(true, 7, 7));
    assert(BridgeTransitionObserved(true, 8, 7));
    assert(!BridgeTransitionObserved(false, 8, 7));

    // The existing fallback remains valid for transitions that are not led by
    // an OS output-color-space change (for example an in-game path).
    assert(!NeedsCommittedOffPresent(false));
    assert(NeedsCommittedOffPresent(true));
    assert(QuiesceCommitted(false, false, false));
    assert(QuiesceCommitted(true, true, true));

    // After ResizeBuffers, two complete fresh-domain frames are still required.
    for (std::uint64_t g = 1; g < 10000; ++g) {
        std::uint32_t c = 0;
        c = NextFreshFrameCount(true, g, ~0ull, 100, 0, c);
        assert(c == 1);
        c = NextFreshFrameCount(true, g, g, 101, 100, c);
        assert(c == 2);
        assert(WarmupComplete(c));
        assert(CanEnable(true, g, g));
    }

    std::puts("PASS HDR display-domain pre-Present guard: output HDR flip holds FG off before ResizeBuffers; bridge generation handoff plus two fresh-domain frames retained");
    return 0;
}
