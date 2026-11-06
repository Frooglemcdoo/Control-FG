#pragma once
namespace character_surface {
#include "../build/character_surface.generated.h"
inline constexpr unsigned kind=0x73460000;
inline std::atomic<unsigned> phase{0},matched{0},ready{0};
inline std::atomic<uint64_t> binds[9]{},applied[9]{};
inline GUID VariantGuid(unsigned index) noexcept {
    return GUID{0xf2713460+index,0x71b4,0x4aea,{0xa7,0x31,0xe6,0x31,0x44,0xc2,0x12,0x54}};
}
template<class Create>
inline void Observe(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,
                    IUnknown* native,Create create) noexcept {
    if(!create || !native || !desc || !desc->PS.pShaderBytecode ||
       desc->PS.BytecodeLength!=sizeof(kCharacterSurface0) ||
       std::memcmp(desc->PS.pShaderBytecode,kCharacterSurface0,sizeof(kCharacterSurface0)))return;
    ID3D12PipelineState* owner=nullptr;
    if(FAILED(native->QueryInterface(IID_PPV_ARGS(&owner))))return;
    unsigned tag=0;UINT size=sizeof(tag);
    if(SUCCEEDED(owner->GetPrivateData(kKindGuid,&size,&tag)) && tag==kind){owner->Release();return;}
    ++matched;
    ID3D12PipelineState* variants[6]{};
    HRESULT hr=S_OK;
    for(unsigned i=0;i<6 && SUCCEEDED(hr);++i){
        auto copy=*desc;copy.PS=kCharacterSurfaceVariants[i];copy.CachedPSO={nullptr,0};
        hr=create(device,&copy,IID_PPV_ARGS(&variants[i]));
    }
    for(unsigned i=0;i<6 && SUCCEEDED(hr);++i)hr=owner->SetPrivateDataInterface(VariantGuid(i),variants[i]);
    if(SUCCEEDED(hr))hr=owner->SetPrivateData(kKindGuid,sizeof(kind),&kind);
    if(SUCCEEDED(hr))++ready;
    else for(unsigned i=0;i<6;++i)owner->SetPrivateDataInterface(VariantGuid(i),nullptr);
    for(auto* p:variants)if(p)p->Release();
    Trace("CHARACTER_SURFACE_R346 matched=%u ready=%u hr=0x%08lX",matched.load(),ready.load(),static_cast<unsigned long>(hr));
    owner->Release();
}
}
