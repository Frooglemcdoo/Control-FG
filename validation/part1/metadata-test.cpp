#include "../../src/rr_part1_metadata.h"
#include <cassert>
#include <iostream>
#include <map>
using namespace control_rr_part1;
int main() {
    const std::map<std::uint64_t,std::uint64_t> valid{
        {0x1060,0x2000},{0x20F0,0x3000},{0x3028,8},{0x3030,65536*8},{0x3048,0xABC000}};
    auto memory=valid;
    unsigned calls=0;
    auto read=[&](std::uint64_t p,std::uint64_t& v) {
        ++calls;
        auto i=memory.find(p);
        if(i==memory.end()) return false;
        v=i->second; return true;
    };
    Metadata out{};
    auto run=[&](std::uint64_t r,std::uint32_t id,Result expected) {
        out.buffer=123;
        assert(ReadMetadata(read,r,id,out)==expected);
        if(expected!=Result::Candidate) assert(out.buffer==0 && out.renderer==0 && out.bytes==0);
    };
    for(unsigned id=0;id<65536;++id) {
        run(0x1000,id,Result::Candidate);
        assert(out.owner==0x2000 && out.buffer==0x3000 && out.elements==65536);
        assert(out.materialOffset==static_cast<std::uint64_t>(id)*8);
    }
    calls=0;run(0,0,Result::InvalidArgument);run(0x1000,65536,Result::InvalidArgument);assert(calls==0);
    for(auto entry:valid) {
        memory=valid;memory.erase(entry.first);run(0x1000,0,Result::Unreadable);
    }
    for(auto p:{0x1060,0x20F0}) { memory=valid;memory[p]=0;run(0x1000,0,Result::NullPointer); }
    for(auto stride:{0,4,16}) {memory=valid;memory[0x3028]=stride;run(0x1000,0,Result::BadStride);}
    for(auto size:{0,1,9}) {memory=valid;memory[0x3030]=size;run(0x1000,0,Result::BadSize);}
    memory=valid;memory[0x3030]=8;run(0x1000,0,Result::Candidate);run(0x1000,1,Result::MaterialOutOfRange);
    memory=valid;memory[0x3048]=0;run(0x1000,0,Result::Candidate);assert(out.gpuAddress==0);
    constexpr auto max=(std::numeric_limits<std::uint64_t>::max)();
    calls=0;run(max,0,Result::Unreadable);run(max-0x60,0,Result::Unreadable);assert(calls==0);
    memory=valid;memory[0x1060]=max;run(0x1000,0,Result::Unreadable);
    memory=valid;memory[0x20F0]=max;run(0x1000,0,Result::Unreadable);
    std::cout << "PASS: 65536 material IDs; missing/null fields; layout; bounds; overflow; output clearing\n";
}
