#pragma once
#include <atomic>
#include "rr_gi_stage_policy.h"
#include <cstring>
#include <new>
#include "../build/gi_motion_history.generated.h"
// Immutable GPU objects, never copies of the engine's shader/lock/cache records.
namespace control_rr_gi {
struct Bundle {ID3D12PipelineState* native=nullptr;ID3D12PipelineState* adaptive=nullptr;ID3D12PipelineState* noHistory=nullptr;};
inline std::atomic<Bundle*> bundles[8]{};
inline std::atomic<unsigned> reserved{0},ready{0},failed{0};
inline Bundle* Find(ID3D12PipelineState* native) noexcept {
 if(!native)return nullptr;
 for(auto& slot:bundles){auto* b=slot.load(std::memory_order_acquire);if(b&&b->native==native)return b;}
 return nullptr;
}
inline bool Target(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
 __try {return desc&&desc->pRootSignature&&desc->CS.pShaderBytecode&&
  desc->CS.BytecodeLength==sizeof(kGINativeHistoryShader)&&
  !std::memcmp(desc->CS.pShaderBytecode,kGINativeHistoryShader,sizeof(kGINativeHistoryShader));}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class Create> void Capture(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* result,Create create) noexcept {
 if(!device||!result||!create||!Target(desc))return;
 ID3D12PipelineState* native=nullptr;
 if(FAILED(result->QueryInterface(IID_PPV_ARGS(&native)))||!native)return;
 if(Find(native)){native->Release();return;}
 const auto index=reserved.fetch_add(1,std::memory_order_relaxed);
 if(index>=8){native->Release();return;}
 auto* b=new(std::nothrow) Bundle{};
 if(!b){native->Release();return;}
 b->native=native;
 auto replacement=*desc;replacement.CachedPSO={};replacement.CS={kGIMotionHistoryShader,sizeof(kGIMotionHistoryShader)};
 const HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&b->adaptive));
 if(FAILED(hr)||!b->adaptive){if(b->adaptive)b->adaptive->Release();native->Release();delete b;++failed;
  Log("GI_PIPELINE_R213 ready=0 hr=0x%08lX native_fallback=1",static_cast<unsigned long>(hr));return;}
 replacement.CS={kGINoHistoryShader,sizeof(kGINoHistoryShader)};
 const HRESULT noHistoryResult=create(device,&replacement,IID_PPV_ARGS(&b->noHistory));
 if(FAILED(noHistoryResult)||!b->noHistory){
  if(b->noHistory)b->noHistory->Release();b->adaptive->Release();native->Release();delete b;++failed;
  Log("GI_PIPELINE_R213 ready=0 no_history_hr=0x%08lX native_fallback=1",static_cast<unsigned long>(noHistoryResult));return;
 }
 bundles[index].store(b,std::memory_order_release);++ready;
 Log("GI_PIPELINE_R213 ready=1 native=%p adaptive=%p native_root_preserved=1 engine_record_copy=0",native,b->adaptive);
}
template<class Set,class DispatchFn> void Dispatch(ID3D12PipelineState* observed,unsigned count,bool& used,
 ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z,Set set,DispatchFn dispatch) {
 auto* b=Find(observed);
 auto* replacement=b?(historyDisabled?b->noHistory:b->adaptive):nullptr;
 if(!replacement||count!=1){dispatch(list,x,y,z);return;}
 // Called only after DeviceUtil commits its native PSO and bindings.
 __try {set(list,replacement);dispatch(list,x,y,z);used=true;}
 __finally {set(list,observed);}
}
}
