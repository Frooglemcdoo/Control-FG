#include "../src/native_lighting_live_state.h"
#include <cassert>
#include <iostream>
int main(){
    using namespace native_lighting;
    LiveSelection live;
    assert(LiveSelection::GetMode(live.Read())==Mode::Native);
    const auto first=live.Select(Mode::DisableBakedGi);
    assert(Disables(LiveSelection::GetMode(first),kGiBit));
    assert(!Disables(LiveSelection::GetMode(first),kDirectBit));
    assert(live.Observe(first,kGiBit));
    const auto second=live.Select(Mode::DisableDirect);
    assert(!live.Observe(first,kDirectBit)); // Old worker cannot confirm a new selection.
    assert(LiveSelection::Observed(live.Read())==0);
    assert(live.Observe(second,kDirectBit));assert(live.Observe(second,kGiBit));
    assert(LiveSelection::Observed(live.Read())==kAllBits);
    const auto native=live.Select(Mode::Native);
    assert(!Disables(LiveSelection::GetMode(native),kGiBit));
    assert(!Disables(LiveSelection::GetMode(native),kDirectBit));
    assert(LiveSelection::Observed(live.Read())==0);
    for (auto mode : {Mode::DisableDirectDiffuse, Mode::DisableDirectSpecular, Mode::BypassShadows, Mode::DisableLightPattern}) {
        const auto old = live.Read(); const auto next = live.Select(mode);
        assert(LiveSelection::GetMode(next) == mode);
        assert(Disables(mode, kDirectBit) && !Disables(mode, kGiBit));
        assert(!live.Observe(old, kDirectBit));
        assert(live.Observe(next, kDirectBit));
        assert(LiveSelection::Observed(live.Read()) == kDirectBit);
        assert(RequiredVariant(mode) != kDirectBit);
    }
    std::cout<<"PASS: live modes, return to native, stale-recording rejection\n";
}
