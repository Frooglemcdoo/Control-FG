#pragma once
// Shared by direct creation and successful native pipeline-library loads.
// Only PSO setup changes: there is no per-bind or per-dispatch tracking.
#include "../build/spec_serial_lifetimes.generated.h"
namespace spec_serial {
inline std::atomic<unsigned long long> directMatched{0},libraryMatched{0};
inline std::atomic<unsigned long long> directCreated{0},libraryCreated{0},failures{0};
inline constexpr char sourceSha[]="4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9";
template<class Create>
inline bool TryCreate(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
    REFIID iid,void** out,Create original,bool library,HRESULT& result) noexcept {
    if(!device||!desc||!out||!original||!desc->CS.pShaderBytecode||desc->CS.BytecodeLength!=42408u)return false;
    char sha[65]{};
    if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,sha)||
       std::strcmp(sha,sourceSha))return false;
    (library?libraryMatched:directMatched).fetch_add(1,std::memory_order_relaxed);
    auto replacement=*desc;
    replacement.CS={kSpecSerialLifetimesShader,sizeof(kSpecSerialLifetimesShader)};
    replacement.CachedPSO={nullptr,0};
    void* candidate=nullptr;
    result=original(device,&replacement,iid,&candidate);
    const bool ready=SUCCEEDED(result)&&candidate;
    if(ready){
        *out=candidate;
        (library?libraryCreated:directCreated).fetch_add(1,std::memory_order_relaxed);
    }else{
        if(candidate)reinterpret_cast<IUnknown*>(candidate)->Release();
        failures.fetch_add(1,std::memory_order_relaxed);
    }
    Log("SPEC_SERIAL_LIFETIMES build=R174 route=%s created=%u hr=0x%08lX source_sha=%s area_guards=2 moved_loads=2 samples=unchanged dispatch=unchanged",
        library?"library":"direct",unsigned(ready),static_cast<unsigned long>(result),sourceSha);
    return ready;
}
}
