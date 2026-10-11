#include "../src/pt_glass_capture_policy.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>

static void Put(std::vector<unsigned char>& b, size_t offset, std::uint32_t v) {
    std::memcpy(b.data()+offset,&v,4);
}
int main() {
    for (unsigned i=0;i<32;++i) {
        char name[256]{};
        std::snprintf(name,sizeof(name),
            "research/hit-distance/evidence/original-shaders/rt_reflection-%03u.dxbc",i);
        std::ifstream stream(name,std::ios::binary);
        assert(stream.good());
        std::vector<unsigned char> b((std::istreambuf_iterator<char>(stream)),{});
        assert(!PTGlassCapture::IsGlassLibrary(b.data(),b.size()));
    }
    // A minimal container with library kind 6 and the target RDAT export.
    std::vector<unsigned char> glass(100,0);
    std::memcpy(glass.data(),"DXBC",4);Put(glass,24,100);Put(glass,28,2);
    Put(glass,32,40);Put(glass,36,64);
    std::memcpy(glass.data()+40,"DXIL",4);Put(glass,44,16);Put(glass,48,0x00060063);
    std::memcpy(glass.data()+64,"RDAT",4);Put(glass,68,28);
    std::memcpy(glass.data()+72,"transparentRayGeneration",24);
    assert(PTGlassCapture::IsGlassLibrary(glass.data(),glass.size()));
    for(size_t size=0;size<glass.size();++size)
        assert(!PTGlassCapture::IsGlassLibrary(glass.data(),size));
    auto invalid=glass;Put(invalid,48,0x00050050); // compute, same string
    assert(!PTGlassCapture::IsGlassLibrary(invalid.data(),invalid.size()));
    invalid=glass;Put(invalid,68,0xffffffff); // part length overflow
    assert(!PTGlassCapture::IsGlassLibrary(invalid.data(),invalid.size()));
    invalid=glass;Put(invalid,36,0xfffffff0); // part offset overflow
    assert(!PTGlassCapture::IsGlassLibrary(invalid.data(),invalid.size()));
    invalid=glass;Put(invalid,28,0xffffffff); // offset table overflow
    assert(!PTGlassCapture::IsGlassLibrary(invalid.data(),invalid.size()));
    assert(!PTGlassCapture::IsGlassLibrary(nullptr,100));
    std::puts("PASS: 32 native libraries excluded; glass export selected; malformed bounds rejected.");
}
