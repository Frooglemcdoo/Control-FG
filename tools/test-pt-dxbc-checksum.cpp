#define NOMINMAX
#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "../src/pt_surface_composition.h"
#include <cassert>
#include <cstdio>
using Microsoft::WRL::ComPtr;
int main(){
    const char source[]="float4 main(float4 position:SV_Position):SV_Target {return position*0.5;}";
    ComPtr<ID3DBlob> shader,error;
    assert(SUCCEEDED(D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"main","ps_5_1",0,0,&shader,&error)));
    std::array<unsigned char,128> payload{};
    for(unsigned n=0;n<payload.size();++n)payload[n]=static_cast<unsigned char>(n);
    for(unsigned size=1;size<=payload.size();++size){
        ComPtr<ID3DBlob> container;
        assert(SUCCEEDED(D3DSetBlobPart(shader->GetBufferPointer(),shader->GetBufferSize(),D3D_BLOB_PRIVATE_DATA,0,payload.data(),size,&container)));
        const auto* bytes=static_cast<const unsigned char*>(container->GetBufferPointer());
        const auto sum=PTDxbc::Checksum(bytes+20,static_cast<unsigned>(container->GetBufferSize()-20));
        assert(!std::memcmp(sum.data(),bytes+4,16));
        std::vector<unsigned char> output;
        assert(!PTSurfaceComposition::Patch(bytes,container->GetBufferSize(),PTSurfaceComposition::Profiles[0].sha,output)&&output.empty());
    }
    std::puts("PASS DXBC retail checksum against compiler-produced containers across final-block lengths; unreviewed shader rejected.");
}
