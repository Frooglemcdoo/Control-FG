#include "../src/spot_shadow_resolution_policy.h"
#include "../src/native_lighting_live_state.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace native_lighting;
    using namespace native_lighting::spot_resolution;
    Extent atlas{};
    assert(ExpandAtlas({16384,2048},atlas));
    assert(atlas.width==16384 && atlas.height==8192);
    assert(!ExpandAtlas({16384,8192},atlas));
    assert(!ExpandAtlas({1024,1024},atlas));
    assert(ExpandAtlas({16384,4096},atlas) && atlas.height==16384);
    assert(ExpandAtlas({16384,2048},atlas));
    for (int size : {256,512,1024,2048}) {
        auto tile=TileRequest({size,size},true,atlas);
        assert(tile.width==2*size && tile.height==2*size);
        tile=TileRequest({size,size},false,atlas);
        assert(tile.width==size && tile.height==size);
    }
    assert(TileRequest({8192,8192},true,atlas).width==8192);
    assert(TileRequest({-1,-1},true,atlas).width==-1);
    assert(TileRequest({1024,512},true,atlas).height==512);
    assert(TileRequest({1000,1000},true,atlas).width==1000);
    assert(!Disables(Mode::SpotResolution2x,kDirectBit));
    assert(!Disables(Mode::SpotResolution2x,kGiBit));
    LiveSelection live;
    auto old=live.Select(Mode::SpotResolution2x);
    auto next=live.Select(Mode::Native);
    assert(!live.Observe(old,kDirectBit));
    assert(live.Observe(next,kDirectBit));
    std::cout << "PASS: atlas limits, 2x tile requests, native restoration, unchanged shader selection\n";
}
