#include "../../src/rr_part1_diagnostic.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace control_rr_part1;
int main() {
    const std::map<std::uint64_t,std::uint64_t> baseline{{0x1060,0x2000},{0x20f0,0x3000},
        {0x3028,8},{0x3030,1024},{0x3048,0xabc000},{0x3040,0x4000},{0x4014,2}};
    auto memory=baseline; unsigned reads=0; bool mutate=false;
    auto read=[&](std::uint64_t a,void* out,std::size_t n) {
        ++reads;
        if(mutate && reads==8) memory[0x3048]+=8;
        auto i=memory.find(a);if(i==memory.end())return false;
        assert(n==4 || n==8);std::memcpy(out,&i->second,n);return true;
    };
    auto good=Observe(read,0x1000);
    assert(good.result==Result::Candidate && good.repeatedReadMatched && good.mappingReadable && good.mappingMode==2);
    assert(!PairMatched(good,good));good.frameMatched=true;assert(PairMatched(good,good));
    auto changed=good;changed.metadata.buffer+=8;assert(!PairMatched(good,changed));
    changed=good;changed.metadata.gpuAddress+=8;assert(!PairMatched(good,changed));
    changed=good;changed.mappingMode=1;assert(!PairMatched(good,changed));
    changed=good;changed.mappingObject+=8;assert(!PairMatched(good,changed));
    changed=good;changed.repeatedReadMatched=false;assert(!PairMatched(good,changed));
    changed=good;changed.mappingReadable=false;assert(!PairMatched(good,changed));
    for(auto address:{0x3040,0x4014}) {memory=baseline;memory.erase(address);auto s=Observe(read,0x1000);assert(!s.mappingReadable);}
    memory=baseline;memory[0x3040]=0;assert(!Observe(read,0x1000).mappingReadable);
    memory=baseline;memory[0x3040]=UINT64_MAX;assert(!Observe(read,0x1000).mappingReadable);
    memory=baseline;reads=0;mutate=true;assert(!Observe(read,0x1000).repeatedReadMatched);
    memory.clear();reads=0;mutate=false;auto s=Observe(read,0x1000);assert(s.result==Result::Unreadable && reads==1);
    std::cout<<"PASS: Part1 repeat/pair checks, mutation, inaccessible mapping, frame rejection\n";
}
