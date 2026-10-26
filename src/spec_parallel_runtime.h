#pragma once
// Included inside native_lighting after PipelineIdentity and CreateCompute.
namespace spec_parallel {
#include "../build/spec_parallel.generated.h"
inline const GUID kVariant = {0x3517d501,0x6d9a,0x43b9,{0x93,0x65,0x1e,0x6d,0x70,0x01,0x00,0x01}};
inline const GUID kTagged = {0x3517d501,0x6d9a,0x43b9,{0x93,0x65,0x1e,0x6d,0x70,0x01,0x00,0x02}};
inline const GUID kBound = {0x3517d501,0x6d9a,0x43b9,{0x93,0x65,0x1e,0x6d,0x70,0x01,0x00,0x03}};
inline constexpr char kNativeSha[]="4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9";
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native,CreateCompute create) noexcept {
    if(!device||!desc||!native||!create||desc->CS.BytecodeLength!=42408)return;
    ID3D12PipelineState* owner=nullptr;
    if(FAILED(native->QueryInterface(IID_PPV_ARGS(&owner))))return;
    PipelineIdentity identity{};UINT size=sizeof(identity);
    if(FAILED(owner->GetPrivateData(kIdentityGuid,&size,&identity))||size!=sizeof(identity)||std::strcmp(identity.sha,kNativeSha)){owner->Release();return;}
    auto replacement=*desc;replacement.CS={kSpecParallelShader,sizeof(kSpecParallelShader)};replacement.CachedPSO={nullptr,0};
    ID3D12PipelineState* alternate=nullptr;
    HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&alternate));
    if(SUCCEEDED(hr)&&!alternate)hr=E_UNEXPECTED;
    if(SUCCEEDED(hr)&&alternate){
        hr=owner->SetPrivateDataInterface(kVariant,alternate);
        const unsigned tagged=1;
        if(SUCCEEDED(hr))hr=owner->SetPrivateData(kTagged,sizeof(tagged),&tagged);
    }
    if(SUCCEEDED(hr))specularParallelReady.store(true,std::memory_order_release);
    Trace("spec_parallel_prepare ready=%u hr=0x%08lX native_sha=%s",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr),kNativeSha);
    if(alternate)alternate->Release();owner->Release();
}
inline void Bind(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) noexcept {
    unsigned tagged=0;UINT size=sizeof(tagged);
    if(pso && (FAILED(pso->GetPrivateData(kTagged,&size,&tagged))||size!=sizeof(tagged)))tagged=0;
    // The command list owns this reference; Reset/other bindings clear it.
    // No global device/list lifetime retention, allocation pool or GPU wait.
    if(FAILED(list->SetPrivateDataInterface(kBound,tagged?pso:nullptr))) {
        specularParallelEnabled.store(false);
        Trace("spec_parallel_disabled reason=binding_metadata_failure");
    }
}
using DispatchFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
using SetFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
inline bool Dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z,DispatchFn dispatch,SetFn set) noexcept {
    if(!specularParallelEnabled.load(std::memory_order_relaxed)||!set||z!=1||!x||!y||
       SpecularSampleRequest(LiveSelection::GetMode(selection.Read()))!=16)return false;
    ID3D12PipelineState* native=nullptr;UINT size=sizeof(native);
    if(FAILED(list->GetPrivateData(kBound,&size,&native))||!native)return false;
    ID3D12PipelineState* alternate=nullptr;size=sizeof(alternate);
    const bool ready=SUCCEEDED(native->GetPrivateData(kVariant,&size,&alternate))&&alternate;
    if(ready){
        set(list,alternate);
        // Each of the 16 Z groups shades one row; its 16 Y lanes shade the samples.
        dispatch(list,x,y,16);
        set(list,native);
        specularParallelDispatches.fetch_add(1,std::memory_order_relaxed);
    }
    if(alternate)alternate->Release();native->Release();return ready;
}
} // namespace spec_parallel
