#include "../../src/rr_frame_slots.h"
#include <cassert>
#include <cstdio>
using namespace control_rr;
int main() {
    for(auto extent:{FrameKey{1,1,7680,4320},FrameKey{1,1,8192,4320},FrameKey{1,1,4320,7680}}){
        FrameSlots high;Lease lease;assert(high.Bind({1,2,3}));assert(high.Begin(extent,lease));
        assert(high.CommandsStarted(lease)&&high.GuidesReady(lease)&&high.TakeEvaluation(lease,extent));
        assert(high.Submit(lease,1)&&high.Collect({1,2,3},1));
        const auto packed=HighResolutionExtent(extent.width,extent.height);
        assert((packed>>32)==extent.width&&static_cast<unsigned>(packed)==extent.height);
        assert(IndependentDiffuseBootstrap(packed,true,false));
        assert(!IndependentDiffuseBootstrap(packed,false,false)&&!IndependentDiffuseBootstrap(packed,true,true));
    }
    for(auto extent:{FrameKey{1,1,0,4320},FrameKey{1,1,8193,4320},FrameKey{1,1,7680,8193},FrameKey{1,1,UINT32_MAX,1}}){
        FrameSlots high;Lease lease;assert(high.Bind({1,2,3}));assert(!high.Begin(extent,lease));
        assert(!HighResolutionExtent(extent.width,extent.height));
    }
    assert(!HighResolutionExtent(3840,2160)&&!HighResolutionExtent(UINT64_MAX,UINT64_MAX));
    FrameSlots slots;FenceKey fence{1,2,3};assert(slots.Bind(fence));
    Lease a,b,c,d;FrameKey frame{1,1,2560,1440};
    assert(slots.Begin(frame,a));assert(!slots.CanEvaluate(a,frame));
    assert(!slots.GuidesReady(a));assert(slots.CommandsStarted(a));assert(slots.GuidesReady(a));
    assert(slots.CanEvaluate(a,frame));assert(!slots.CanEvaluate(a,{2,1,2560,1440}));
    assert(slots.TakeEvaluation(a,frame));assert(!slots.TakeEvaluation(a,frame));
    assert(slots.Submit(a,1));assert(!slots.CanEvaluate(a,frame));
    assert(slots.Begin({2,1,2560,1440},b));assert(slots.CommandsStarted(b));assert(slots.Submit(b,2));
    assert(slots.Begin({3,1,2560,1440},c));assert(slots.CommandsStarted(c));assert(slots.Submit(c,3));
    assert(!slots.Begin({4,1,2560,1440},d));
    assert(!slots.Collect({1,2,99},3));assert(!slots.Bind({1,2,99}));
    assert(slots.Collect(fence,0));assert(!slots.Begin({4,1,2560,1440},d));
    assert(slots.Collect(fence,1));assert(slots.Begin({4,1,2560,1440},d));
    assert(!slots.CommandsStarted(a));assert(slots.Abort(d));assert(!slots.Begin({4,1,2560,1440},d));
    assert(slots.Begin({5,2,1280,720},d));assert(slots.CommandsStarted(d));assert(slots.GuidesReady(d));
    assert(!slots.CanEvaluate(d,{5,1,1280,720}));assert(slots.CanEvaluate(d,{5,2,1280,720}));
    assert(slots.Abort(d));assert(slots.Inspect()[d.index].state==SlotState::Retained);
    assert(slots.Collect(fence,3));assert(slots.Inspect()[d.index].state==SlotState::Retained);
    assert(!slots.Collect(fence,2));assert(!slots.Collect(fence,4));
    assert(!slots.Collect(fence,UINT64_MAX));assert(!slots.Begin({6,2,1280,720},a));
    FrameSlots cycling;assert(cycling.Bind(fence));Lease stale{};
    for(std::uint64_t i=1;i<=100000;++i) {
        Lease lease;assert(cycling.Begin({i,1,2560,1440},lease));assert(!cycling.CommandsStarted(stale));
        assert(cycling.CommandsStarted(lease));assert(cycling.GuidesReady(lease));assert(cycling.Submit(lease,i));
        assert(cycling.Collect(fence,i));stale=lease;
    }
    puts("PASS: bounded slots, stalled/wrong fences, no early reuse, resize epoch, stale leases, retained failures and 100000 retire/reuse cycles");
}
