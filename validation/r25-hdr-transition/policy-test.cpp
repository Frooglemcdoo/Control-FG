#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_hdr_transition_policy.h"

int main() {
    using namespace control_fg_hdr_transition;
    assert(!ResetPending(0,0));
    assert(CanEnable(true,0,0));
    assert(!CanEnable(false,0,0));
    for (std::uint64_t g=1; g<10000; ++g) {
        assert(ResetPending(g,g-1));
        assert(!CanEnable(true,g,g-1));
        assert(!ShouldPublishReset(g,g,false));
        assert(!ShouldPublishReset(g,g+1,true));
        assert(ShouldPublishReset(g,g,true));
        assert(CanEnable(true,g,g));
    }
    std::puts("PASS HDR transition policy: every color-domain generation blocks FG until one successful reset constants submit");
    return 0;
}
