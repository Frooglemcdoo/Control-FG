#include <cassert>
#include <iostream>
#include "../../src/rr_albedo_shader.h"

std::vector<std::vector<uint8_t>> blobs;
const char* Getter(int index) {return reinterpret_cast<const char*>(blobs.at(index).data());}
struct Descriptor {uint64_t unused;uint32_t stage;int32_t id;uint64_t size;};
int main() {
    using namespace RRAlbedoShader;
    blobs.resize(3,std::vector<uint8_t>(32,0));
    for(auto& b:blobs) {std::memcpy(b.data(),"DXBC",4);b[24]=32;}
    blobs[0][31]=1;blobs[1][31]=2;blobs[2][31]=3;
    Descriptor original{0,0,0,32},replacement{0,0,1,32},pixel{0,4,2,32};
    uintptr_t a[11]{},b[11]{};
    a[2]=reinterpret_cast<uintptr_t>(&original);b[2]=reinterpret_cast<uintptr_t>(&replacement);b[10]=reinterpret_cast<uintptr_t>(&pixel);
    residentGetter=&Getter;Result result{};const char* why=nullptr;
    Session live;
    assert(!live.ValidatePair(reinterpret_cast<uintptr_t>(a),reinterpret_cast<uintptr_t>(b),result,&why));
    assert(std::strcmp(why,"native_vertex_bytecode_differs")==0);
    assert(live.VertexEvidence().PairCount()==0 && result.vertexEvidenceIndex==RRVertexEvidence::Missing);
    Session capture(true);
    assert(!capture.ValidatePair(reinterpret_cast<uintptr_t>(a),reinterpret_cast<uintptr_t>(b),result,&why));
    assert(result.vertexEvidenceIndex==0 && capture.VertexEvidence().PairCount()==1);
    capture.VertexEvidence().AddReference({264,4,0x2000000,0x2000002,true,result.vertexEvidenceIndex});
    assert(!capture.ValidatePair(reinterpret_cast<uintptr_t>(a),reinterpret_cast<uintptr_t>(b),result,&why));
    assert(result.vertexEvidenceIndex==0 && capture.PairCount()==1);
    capture.VertexEvidence().AddReference({265,4,0x2000000,0x2000002,true,result.vertexEvidenceIndex});
    // Captured arrays own their bytes, independent of resident program lifetime.
    blobs[0].assign(32,99);
    std::cout << capture.VertexEvidence().Json() << '\n';
    RRVertexEvidence::Store bounded;
    std::vector<uint8_t> big(262144,1);
    assert(bounded.Add(big,big,big)==0);
    assert(bounded.Add(big,big,big)==0); // byte-exact dedup
    big[0]=2;assert(bounded.Add(big,big,big)==1);
    big[0]=3;assert(bounded.Add(big,big,big)==RRVertexEvidence::Missing);
    for(unsigned i=0;i<257;++i) bounded.AddReference({i,0,0,0,false,RRVertexEvidence::Missing});
    std::cout << bounded.Json() << '\n';
    RRVertexEvidence::Store capped;
    std::vector<uint8_t> small(32,0);
    for(unsigned i=0;i<33;++i) {small[0]=static_cast<uint8_t>(i);auto id=capped.Add(small,small,small);assert(id==(i<32?i:RRVertexEvidence::Missing));}
    assert(capped.Add({},small,small)==RRVertexEvidence::Missing);
    std::cout << capped.Json() << '\n';
}
