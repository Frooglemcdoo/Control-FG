#pragma once
namespace volume_temporal {
#include "../build/volume_temporal.generated.h"
inline constexpr unsigned kind=0x73530000;
inline constexpr GUID variantGuid{0xf2713a53,0x71b4,0x4aea,{0xa7,0x31,0xe6,0x31,0x44,0xc2,0x12,0x54}};
inline std::atomic<unsigned> matched{0},ready{0};
inline std::atomic<uint64_t> binds{0},applied{0};
inline bool Enabled(unsigned phase,unsigned quality) noexcept {return phase==4 || (phase==0 && quality==3);}
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native,CreateCompute create) noexcept {
    if(!device||!create||!native||!desc||!desc->CS.pShaderBytecode)return;
    const D3D12_SHADER_BYTECODE original{kTemporalNative,sizeof(kTemporalNative)};
    const auto a=character_surface::Chunk(original,"SHEX"),b=character_surface::Chunk(desc->CS,"SHEX");
    if(!a.pShaderBytecode||!b.pShaderBytecode||a.BytecodeLength!=b.BytecodeLength||std::memcmp(a.pShaderBytecode,b.pShaderBytecode,a.BytecodeLength))return;
    ID3D12PipelineState *owner=nullptr,*variant=nullptr;
    if(FAILED(native->QueryInterface(IID_PPV_ARGS(&owner))))return;
    unsigned old=0;UINT size=sizeof(old);
    if(SUCCEEDED(owner->GetPrivateData(kKindGuid,&size,&old))&&old==kind){owner->Release();return;}
    ++matched;auto changed=*desc;changed.CS={kTemporalNear,sizeof(kTemporalNear)};changed.CachedPSO={nullptr,0};
    HRESULT hr=create(device,&changed,IID_PPV_ARGS(&variant));
    if(SUCCEEDED(hr)){
        hr=owner->SetPrivateDataInterface(variantGuid,variant);
        if(SUCCEEDED(hr))hr=owner->SetPrivateData(kKindGuid,sizeof(kind),&kind);
        if(SUCCEEDED(hr))++ready;else owner->SetPrivateDataInterface(variantGuid,nullptr);
        variant->Release();
    }
    Trace("VOLUME_TEMPORAL_R353 matched=%u ready=%u hr=0x%08lX",matched.load(),ready.load(),static_cast<unsigned long>(hr));
    owner->Release();
}
}
