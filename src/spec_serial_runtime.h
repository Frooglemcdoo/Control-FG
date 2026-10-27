#pragma once
// Shared by direct creation and successful native pipeline-library loads.
// Only PSO setup changes: there is no per-bind or per-dispatch tracking.
#include "../build/spec_serial_lifetimes.generated.h"
#ifdef CONTROL_SPEC_HIT_VIEW
#include "../build/spec_hit_view.generated.h"
#endif
#if defined(CONTROL_SPEC_HIT_VIEW) && defined(CONTROL_RR_COMPONENT_CAPTURE)
#error Hit-view correction and component isolation must be tested separately
#endif
#ifdef CONTROL_RR_COMPONENT_CAPTURE
#include "../build/spec_components.generated.h"
#include "spec_component_policy.h"
#endif
#ifdef CONTROL_SPEC_LOCAL_CAPTURE
#include "../build/spec_local_components.generated.h"
#include "spec_local_policy.h"
#if !defined(CONTROL_SPEC_HIT_VIEW) || defined(CONTROL_RR_COMPONENT_CAPTURE)
#error Local isolation requires hit-view baseline and excludes emission isolation
#endif
#endif
#ifdef CONTROL_SPEC_INDIRECT_CAPTURE
#include "../build/spec_indirect_components.generated.h"
#include "spec_indirect_policy.h"
#if !defined(CONTROL_SPEC_HIT_VIEW) || defined(CONTROL_RR_COMPONENT_CAPTURE) || defined(CONTROL_SPEC_LOCAL_CAPTURE)
#error Indirect isolation requires hit-view baseline and excludes other isolation modes
#endif
#endif
namespace spec_serial {
inline unsigned ComponentMode() noexcept {
#if defined(CONTROL_SPEC_INDIRECT_CAPTURE)
    static const unsigned mode=[](){char value[32]{};const auto n=GetEnvironmentVariableA("CONTROLFG_SPEC_INDIRECT_COMPONENT",value,sizeof(value));
        return n&&n<sizeof(value)?spec_indirect::Parse(value):0u;}();
    return mode;
#elif defined(CONTROL_SPEC_LOCAL_CAPTURE)
    static const unsigned mode=[](){char value[32]{};const auto n=GetEnvironmentVariableA("CONTROLFG_SPEC_LOCAL_COMPONENT",value,sizeof(value));
        return n&&n<sizeof(value)?spec_local::Parse(value):0u;}();
    return mode;
#elif defined(CONTROL_RR_COMPONENT_CAPTURE)
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
#ifdef CONTROL_SPEC_HIT_VIEW
    replacement.CS={kSpecHitViewShader,sizeof(kSpecHitViewShader)};
#endif
#ifdef CONTROL_SPEC_LOCAL_CAPTURE
    const auto localComponent=ComponentMode();
    if(localComponent==1)replacement.CS={kSpecLocalOther,sizeof(kSpecLocalOther)};
    if(localComponent==2)replacement.CS={kSpecLocalLocal,sizeof(kSpecLocalLocal)};
#endif
#ifdef CONTROL_SPEC_INDIRECT_CAPTURE
    const auto indirectComponent=ComponentMode();
    if(indirectComponent==1)replacement.CS={kSpecIndirectDiffuse,sizeof(kSpecIndirectDiffuse)};
    if(indirectComponent==2)replacement.CS={kSpecIndirectEnvironment,sizeof(kSpecIndirectEnvironment)};
#endif
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
    Log("SPEC_SERIAL_LIFETIMES build=R187 route=%s created=%u hr=0x%08lX source_sha=%s area_guards=0 moved_loads=2 samples=unchanged dispatch=unchanged",
        library?"library":"direct",unsigned(ready),static_cast<unsigned long>(result),sourceSha);
#ifdef CONTROL_SPEC_HIT_VIEW
    Log("SPEC_HIT_VIEW build=R187 applied=%u native_fallback=%u extra_rays=0 extra_texture_reads=0 near_zero_fallback=1",unsigned(ready),unsigned(!ready));
#endif
#ifdef CONTROL_SPEC_LOCAL_CAPTURE
    Log("SPEC_LOCAL_COMPONENT build=R187 requested=%s applied=%u native_fallback=%u cap_basis=original_total",spec_local::Name(localComponent),unsigned(ready),unsigned(!ready));
#endif
#ifdef CONTROL_SPEC_INDIRECT_CAPTURE
    Log("SPEC_INDIRECT_COMPONENT build=R187 requested=%s diagnostic_gain=%u applied=%u native_fallback=%u cap_basis=original_total",spec_indirect::Name(indirectComponent),spec_indirect::Gain(indirectComponent),unsigned(ready),unsigned(!ready));
#endif
#ifdef CONTROL_RR_COMPONENT_CAPTURE
    Log("SPEC_COMPONENT build=R187 requested=%s applied=%u native_fallback=%u cap_basis=original_total",spec_component::Name(component),unsigned(ready),unsigned(!ready));
#endif
    return ready;
}
}
