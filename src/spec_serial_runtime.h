#pragma once
// Shared by direct creation and successful native pipeline-library loads.
// Only PSO setup changes: there is no per-bind or per-dispatch tracking.
#include "../build/spec_serial_lifetimes.generated.h"
#ifdef CONTROL_RR_COMPONENT_CAPTURE
#include "../build/spec_components.generated.h"
#include "spec_component_policy.h"
#endif
namespace spec_serial {
inline unsigned ComponentMode() noexcept {
#ifdef CONTROL_RR_COMPONENT_CAPTURE
    // Startup-only selection; cannot alter a live pipeline or persist settings.
    static const unsigned mode=[](){char value[32]{};const auto n=GetEnvironmentVariableA("CONTROLFG_SPEC_COMPONENT",value,sizeof(value));
        return n&&n<sizeof(value)?spec_component::Parse(value):0u;}();
    return mode;
#else
    return 0;
#endif
}
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
#ifdef CONTROL_RR_COMPONENT_CAPTURE
    const auto component=ComponentMode();
    if(component==1)replacement.CS={kSpecComponentEmission,sizeof(kSpecComponentEmission)};
    if(component==2)replacement.CS={kSpecComponentShading,sizeof(kSpecComponentShading)};
#endif
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
    Log("SPEC_SERIAL_LIFETIMES build=R182 route=%s created=%u hr=0x%08lX source_sha=%s area_guards=0 moved_loads=2 samples=unchanged dispatch=unchanged",
        library?"library":"direct",unsigned(ready),static_cast<unsigned long>(result),sourceSha);
#ifdef CONTROL_RR_COMPONENT_CAPTURE
    Log("SPEC_COMPONENT build=R182 requested=%s applied=%u native_fallback=%u cap_basis=original_total",spec_component::Name(component),unsigned(ready),unsigned(!ready));
#endif
    return ready;
}
}
