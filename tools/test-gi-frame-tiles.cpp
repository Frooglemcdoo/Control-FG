#include "../src/gi_reuse/frame_tiles.h"
#include <cassert>
#include <iostream>
int main(){
    using gi_reuse::GIFrameTiles;GIFrameTiles plan;
    for(auto rays:{1u,6u,16u,64u})for(auto size:{1u,16383u,16384u,16385u,8294400u,16777216u}){
        assert(GIFrameTiles::Make(size,1,0,size,rays,plan));
        std::uint64_t next=0,total=0;
        for(std::uint32_t i=0;i<plan.tiles;++i){const auto s=plan.At(i);
            assert(s.first==next&&s.pixels&&s.pixels<=16384&&s.samples<=262144);
            assert(s.samples==s.pixels*rays);next+=s.pixels;total+=s.samples;
        }
        assert(next==size&&total==std::uint64_t(size)*rays&&!plan.At(plan.tiles).pixels);
    }
    assert(GIFrameTiles::Make(2560,1440,12345,20001,64,plan));
    assert(plan.At(0).first==12345&&plan.At(plan.tiles-1).first+plan.At(plan.tiles-1).pixels==32346);
    for(auto rays:{0u,65u})assert(!GIFrameTiles::Make(10,10,0,100,rays,plan));
    assert(!GIFrameTiles::Make(0,1,0,1,1,plan));
    assert(!GIFrameTiles::Make(UINT32_MAX,UINT32_MAX,0,1,1,plan));
    assert(!GIFrameTiles::Make(100,100,9999,2,1,plan));
    assert(!GIFrameTiles::Make(100,100,10000,1,1,plan));
    assert(!GIFrameTiles::Make(100,100,0,0,1,plan));
    std::cout<<"PASS full-frame tile coverage, tails, ray budget, subregions and invalid extents\n";
}
