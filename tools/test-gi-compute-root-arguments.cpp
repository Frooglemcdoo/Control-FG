#include "../src/gi_reuse/compute_root_arguments.h"
#include <cassert>
#include <iostream>
using namespace gi_reuse;
struct Sink {
    unsigned descriptors=0, spans=0, words=0;
    std::array<RootArgumentKind,64> kinds{};
    std::array<std::uint64_t,64> addresses{}, masks{};
    std::array<std::array<std::uint32_t,64>,64> values{};
    void Descriptor(unsigned i,RootArgumentKind kind,std::uint64_t address) noexcept {
        ++descriptors; kinds[i]=kind; addresses[i]=address;
    }
    void Constants(unsigned i,unsigned offset,unsigned count,const std::uint32_t* data) noexcept {
        ++spans; words+=count;
        for(unsigned n=0;n<count;++n){masks[i]|=std::uint64_t(1)<<(offset+n);values[i][offset+n]=data[n];}
    }
};
int main() {
    ComputeRootArguments state; Sink sink;
    assert(!state.Replay(sink)); state.Root(1);
    state.Descriptor(0,RootArgumentKind::Table,100);
    state.Descriptor(1,RootArgumentKind::CBV,256);
    state.Descriptor(2,RootArgumentKind::SRV,512);
    state.Descriptor(3,RootArgumentKind::UAV,0); // Defined null address is not unknown.
    std::uint32_t values[]={5,6,7};
    state.Constants(4,0,1,values); state.Constants(4,2,2,values+1);
    state.Constants(4,63,1,values+2);
    state.Root(1); // Same root preserves arguments.
    assert(state.Replay(sink));
    assert(sink.descriptors==4 && sink.spans==3 && sink.words==4);
    assert(sink.kinds[0]==RootArgumentKind::Table && sink.addresses[1]==256);
    assert(sink.kinds[2]==RootArgumentKind::SRV && sink.kinds[3]==RootArgumentKind::UAV);
    assert(sink.masks[4]==((std::uint64_t(1)<<63)|13));
    auto frozen=state;
    state.HeapChange(); sink={}; assert(state.Replay(sink));
    assert(sink.descriptors==3 && sink.kinds[0]==RootArgumentKind::None && sink.words==4);
    state.Root(2); sink={}; assert(state.Replay(sink)); assert(!sink.descriptors && !sink.words);
    state.Invalidate(); state.Root(3); assert(!state.Ready());
    sink={}; assert(frozen.Replay(sink) && sink.descriptors==4 && sink.words==4);
    state.Reset(); state.Root(1); state.Constants(0,63,2,values); assert(!state.Ready());
    state.Reset(); state.Root(1); state.Descriptor(64,RootArgumentKind::CBV,256); assert(!state.Ready());
    state.Reset(); state.Root(1); state.Descriptor(0,RootArgumentKind::CBV,256);
    state.Descriptor(0,RootArgumentKind::Table,5); assert(!state.Ready());
    state.Reset(); state.Root(1); state.Constants(0,0,1,nullptr); assert(!state.Ready());
    state.Reset(); state.Root(1); state.Constants(0,64,0,nullptr); assert(state.Ready());
    // A mutable recording and a frozen capture cannot share constant storage.
    state.Reset(); state.Root(1); state.Constants(0,0,3,values); frozen=state;
    values[0]=99; state.Constants(0,0,1,values); sink={}; frozen.Replay(sink);
    assert(sink.values[0][0]==5 && sink.values[0][1]==6 && sink.values[0][2]==7);
    std::cout<<"PASS compute arguments: all descriptor kinds, sparse constants, root/heap invalidation, reset, immutable capture and bounded refusal\n";
}
