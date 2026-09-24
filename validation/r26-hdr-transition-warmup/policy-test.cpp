#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_hdr_transition_policy.h"

int main() {
    using namespace control_fg_hdr_transition;
    static_assert(kFreshFramesRequired == 2, "r26 needs two fresh frames before FG re-enable");
    assert(!TransitionPending(0,0));
    assert(CanEnable(true,0,0));
    assert(!CanEnable(false,0,0));

    for (std::uint64_t g=1; g<10000; ++g) {
        assert(TransitionPending(g,g-1));
        assert(!CanEnable(true,g,g-1));

        std::uint32_t c=0;
        c=NextFreshFrameCount(false,g,~0ull,100,0,c);
        assert(c==0);
        c=NextFreshFrameCount(true,g,~0ull,100,0,c);
        assert(c==1);
        assert(!WarmupComplete(c));
        c=NextFreshFrameCount(true,g,g,101,100,c);
        assert(c==2);
        assert(WarmupComplete(c));
        assert(CanEnable(true,g,g));

        // A gap or missing input restarts the fresh-domain pair.
        c=NextFreshFrameCount(true,g,g,103,101,c);
        assert(c==1);
        c=NextFreshFrameCount(false,g,g,104,103,c);
        assert(c==0);
    }
    std::puts("PASS HDR transition warmup policy: eOff transition requires two consecutive fully tagged new-domain frames; no forced common-constants reset");
    return 0;
}
