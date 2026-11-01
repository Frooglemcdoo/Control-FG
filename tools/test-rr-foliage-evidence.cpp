#include <cstdio>
#include <stdexcept>
#include "../src/rr_albedo_shader.h"
#include "../src/rr_foliage_evidence.h"
static void Require(bool value){if(!value)throw std::runtime_error("Foliage exact evidence contract failed");}
#ifdef _WIN32
static std::vector<unsigned char> blobs[4];
static const char* __cdecl GetBlob(int id){return id>=0&&id<4?reinterpret_cast<const char*>(blobs[id].data()):nullptr;}
struct FakeShader {
    std::array<unsigned char,0xA8> shader{};
    std::array<unsigned char,24> vs{},ps{};
    FakeShader(int vertex,int pixel){
        uint32_t stage=0;uint64_t size=blobs[vertex].size();
        std::memcpy(vs.data()+8,&stage,4);std::memcpy(vs.data()+12,&vertex,4);std::memcpy(vs.data()+16,&size,8);
        stage=4;size=blobs[pixel].size();std::memcpy(ps.data()+8,&stage,4);std::memcpy(ps.data()+12,&pixel,4);std::memcpy(ps.data()+16,&size,8);
        uintptr_t address=reinterpret_cast<uintptr_t>(vs.data());std::memcpy(shader.data()+0x10,&address,sizeof(address));
        address=reinterpret_cast<uintptr_t>(ps.data());std::memcpy(shader.data()+0x50,&address,sizeof(address));
    }
    uintptr_t Address()const{return reinterpret_cast<uintptr_t>(shader.data());}
};
static void NativeSessionTests(){
    blobs[0]={kRRFoliageVertex,kRRFoliageVertex+sizeof(kRRFoliageVertex)};
    blobs[1]={kRRFoliageOriginalPixel,kRRFoliageOriginalPixel+sizeof(kRRFoliageOriginalPixel)};
    blobs[2]={kRRFoliageReplayPixel,kRRFoliageReplayPixel+sizeof(kRRFoliageReplayPixel)};
    blobs[3]=blobs[0];RRAlbedoShader::residentGetter=&GetBlob;
    FakeShader original(0,1),replay(3,2);RRAlbedoShader::Result result{};const char* reason=nullptr;
    {RRAlbedoShader::Session s;Require(s.ValidatePair(original.Address(),replay.Address(),result,&reason,&kRRFoliageEvidence));Require(result.targetCount==1);
     const auto copied=s.CopiedBytes();Require(s.ValidatePair(original.Address(),replay.Address(),result,&reason,&kRRFoliageEvidence));Require(s.CopiedBytes()==copied);}
    blobs[1][100]^=1;
    {RRAlbedoShader::Session s;Require(s.ValidatePair(original.Address(),replay.Address(),result,&reason));
     Require(!s.ValidatePair(original.Address(),replay.Address(),result,&reason,&kRRFoliageEvidence));Require(!std::strcmp(reason,"native_exact_original_pixel_mismatch"));}
    blobs[1][100]^=1;blobs[3][100]^=1;
    {RRAlbedoShader::Session s;Require(!s.ValidatePair(original.Address(),replay.Address(),result,&reason,&kRRFoliageEvidence));Require(!std::strcmp(reason,"native_vertex_bytecode_differs"));}
    blobs[3][100]^=1;blobs[2][100]^=1;
    {RRAlbedoShader::Session s;Require(!s.ValidatePair(original.Address(),replay.Address(),result,&reason,&kRRFoliageEvidence));Require(!std::strcmp(reason,"native_exact_evidence_mismatch"));}
    blobs[2][100]^=1;
    {RRAlbedoShader::Session s;Require(RRValidateFoliagePair(s,original.Address(),replay.Address(),result,&reason));}
    blobs[1]={kRRFoliageOriginalPixelR266,kRRFoliageOriginalPixelR266+sizeof(kRRFoliageOriginalPixelR266)};
    FakeShader secondOriginal(0,1);
    {RRAlbedoShader::Session s;Require(RRValidateFoliagePair(s,secondOriginal.Address(),replay.Address(),result,&reason));
     const auto copied=s.CopiedBytes();Require(RRValidateFoliagePair(s,secondOriginal.Address(),replay.Address(),result,&reason));Require(s.CopiedBytes()==copied);}
    blobs[1][100]^=1;
    {RRAlbedoShader::Session s;Require(!RRValidateFoliagePair(s,secondOriginal.Address(),replay.Address(),result,&reason));}
    blobs[1]={kRRFoliageOriginalPixel,kRRFoliageOriginalPixel+sizeof(kRRFoliageOriginalPixel)};
    RRAlbedoShader::ExactEvidence wrong=kRRFoliageEvidence;wrong.replayPixelSize--;
    {RRAlbedoShader::Session s;Require(s.ValidatePair(original.Address(),replay.Address(),result,&reason,&kRRFoliageEvidence));Require(!s.ValidatePair(original.Address(),replay.Address(),result,&reason,&wrong));}
}
#endif
int main(){try{
    const char* reason=nullptr;RRAlbedoShader::Contract contract{};
    Require(RRAlbedoShader::ParseVertex(kRRFoliageVertex,sizeof(kRRFoliageVertex),&reason));
    Require(RRAlbedoShader::ParsePixelContract(kRRFoliageReplayPixel,sizeof(kRRFoliageReplayPixel),contract,&reason));Require(contract.targetCount==1);
    std::vector<uint8_t> data(kRRFoliageReplayPixel,kRRFoliageReplayPixel+sizeof(kRRFoliageReplayPixel));
    Require(RRAlbedoShader::ExactBytes(data,kRRFoliageReplayPixel,sizeof(kRRFoliageReplayPixel)));
    for(size_t i=0;i<data.size();++i){data[i]^=1;Require(!RRAlbedoShader::ExactBytes(data,kRRFoliageReplayPixel,sizeof(kRRFoliageReplayPixel)));data[i]^=1;}
#ifdef _WIN32
    NativeSessionTests();
#endif
    std::puts("PASS foliage native vertex/replay contract and exact byte rejection; Windows additionally verifies resident Session, cache separation and native PS rejection");
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
