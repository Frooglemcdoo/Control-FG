#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_hdr_transition_policy.h"

int main() {
    using namespace control_fg_hdr_transition;
    static_assert(kFreshFramesRequired == 2, "r27 retains two fresh frames after the committed off Present");

    assert(!NeedsCommittedOffPresent(false));
    assert(NeedsCommittedOffPresent(true));
    assert(QuiesceCommitted(false, false, false));
    assert(!QuiesceCommitted(true, false, false));
    assert(!QuiesceCommitted(true, true, false));
    assert(!QuiesceCommitted(true, false, true));
    assert(QuiesceCommitted(true, true, true));

    for (std::uint64_t g=1; g<10000; ++g) {
        std::uint32_t c=0;
        c=NextFreshFrameCount(true,g,~0ull,100,0,c);
        assert(c==1);
        c=NextFreshFrameCount(true,g,g,101,100,c);
        assert(c==2);
        assert(WarmupComplete(c));
        assert(CanEnable(true,g,g));
    }
    std::puts("PASS HDR transition quiesce policy: active FG requires eOff plus one committed real Present before ResizeBuffers; two fresh-domain frames retained after transition");
    return 0;
}
