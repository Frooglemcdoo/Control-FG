#include "../src/pt_primary_pipeline_probe.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
static void Put(std::vector<unsigned char>& b,std::size_t offset,std::uint32_t value){std::memcpy(b.data()+offset,&value,4);}
int main(int argc,char** argv){
    if(argc>1){std::ifstream stream(argv[1],std::ios::binary);assert(stream.good());
        std::vector<unsigned char> b((std::istreambuf_iterator<char>(stream)),{});
        assert(PTPrimaryPipelineProbe::Candidate(b.data(),b.size()));
        std::puts("PASS: recovered native primary shader selected for evidence collection.");}
    std::vector<unsigned char> b(192,0);std::memcpy(b.data(),"DXBC",4);Put(b,24,192);Put(b,28,2);Put(b,32,40);Put(b,36,52);
    std::memcpy(b.data()+40,"SHEX",4);Put(b,44,4);Put(b,48,0x00050050);
    std::memcpy(b.data()+52,"RDEF",4);Put(b,56,132);
    std::memcpy(b.data()+60,"g_tGBuffer1",12);std::memcpy(b.data()+80,"g_tGBuffer2",12);
    std::memcpy(b.data()+100,"deferredlight_constants",24);
    assert(PTPrimaryPipelineProbe::Candidate(b.data(),b.size()));
    for(std::size_t n=0;n<b.size();++n)assert(!PTPrimaryPipelineProbe::Candidate(b.data(),n));
    auto invalid=b;Put(invalid,48,0x00000050);assert(!PTPrimaryPipelineProbe::Candidate(invalid.data(),invalid.size()));
    invalid=b;std::memcpy(invalid.data()+52,"JUNK",4);assert(!PTPrimaryPipelineProbe::Candidate(invalid.data(),invalid.size()));
    invalid=b;Put(invalid,56,0xffffffff);assert(!PTPrimaryPipelineProbe::Candidate(invalid.data(),invalid.size()));
    invalid=b;Put(invalid,36,0xffffffff);assert(!PTPrimaryPipelineProbe::Candidate(invalid.data(),invalid.size()));
    invalid=b;invalid[100]='x';assert(!PTPrimaryPipelineProbe::Candidate(invalid.data(),invalid.size()));
    assert(!PTPrimaryPipelineProbe::Candidate(nullptr,192));
    std::puts("PASS: primary evidence filter requires compute-stage RDEF names and bounded containers; no rendering admission.");
}
