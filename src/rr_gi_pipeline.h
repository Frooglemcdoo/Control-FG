#pragma once
#include <atomic>
#include "rr_gi_stage_policy.h"
#include <cstring>
#include <new>
#include "../build/gi_motion_history.generated.h"
#include "../build/gi_spatial_history.generated.h"
// Immutable GPU objects, never copies of the engine's shader/lock/cache records.
namespace control_rr_gi {
struct Bundle {bool spatial=false;ID3D12PipelineState* bounded=nullptr;ID3D12PipelineState* steady=nullptr;ID3D12PipelineState* native=nullptr;ID3D12PipelineState* adaptive=nullptr;ID3D12PipelineState* noHistory=nullptr;ID3D12PipelineState* outlier=nullptr;ID3D12PipelineState* mask=nullptr;ID3D12PipelineState* zero=nullptr;ID3D12PipelineState* smooth=nullptr;};
inline std::atomic<Bundle*> bundles[8]{};
inline std::atomic<unsigned> reserved{0},ready{0},failed{0};
inline Bundle* Find(ID3D12PipelineState* native) noexcept {
 if(!native)return nullptr;
 for(auto& slot:bundles){auto* b=slot.load(std::memory_order_acquire);if(b&&b->native==native)return b;}
 return nullptr;
}
inline int SpatialVariant(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
 __try {
  if(!desc||!desc->pRootSignature||!desc->CS.pShaderBytecode)return -1;
  if(desc->CS.BytecodeLength==sizeof(kGINativeSpatial0)&&!std::memcmp(desc->CS.pShaderBytecode,kGINativeSpatial0,sizeof(kGINativeSpatial0)))return 0;
  if(desc->CS.BytecodeLength==sizeof(kGINativeSpatial1)&&!std::memcmp(desc->CS.pShaderBytecode,kGINativeSpatial1,sizeof(kGINativeSpatial1)))return 1;
 }__except(EXCEPTION_EXECUTE_HANDLER){}
 return -1;
}
inline bool Target(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
 __try {return desc&&desc->pRootSignature&&desc->CS.pShaderBytecode&&
  desc->CS.BytecodeLength==sizeof(kGINativeHistoryShader)&&
  !std::memcmp(desc->CS.pShaderBytecode,kGINativeHistoryShader,sizeof(kGINativeHistoryShader));}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class Create> void Capture(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* result,Create create) noexcept {
 const int spatial=SpatialVariant(desc);
 if(!device||!result||!create||(!Target(desc)&&spatial<0))return;
 ID3D12PipelineState* native=nullptr;
 if(FAILED(result->QueryInterface(IID_PPV_ARGS(&native)))||!native)return;
 if(Find(native)){native->Release();return;}
 const auto index=reserved.fetch_add(1,std::memory_order_relaxed);
 if(index>=8){native->Release();return;}
 auto* b=new(std::nothrow) Bundle{};
 if(!b){native->Release();return;}
 b->native=native;
 if(spatial>=0){
  b->spatial=true;auto replacement=*desc;replacement.CachedPSO={};
  replacement.CS=spatial==0?D3D12_SHADER_BYTECODE{kGISingleHistorySpatial0,sizeof(kGISingleHistorySpatial0)}:D3D12_SHADER_BYTECODE{kGISingleHistorySpatial1,sizeof(kGISingleHistorySpatial1)};
  const HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&b->adaptive));
  if(FAILED(hr)||!b->adaptive){if(b->adaptive)b->adaptive->Release();native->Release();delete b;++failed;return;}
  replacement.CS=spatial==0?D3D12_SHADER_BYTECODE{kGIBoundedHistorySpatial0,sizeof(kGIBoundedHistorySpatial0)}:D3D12_SHADER_BYTECODE{kGIBoundedHistorySpatial1,sizeof(kGIBoundedHistorySpatial1)};
  const HRESULT boundedResult=create(device,&replacement,IID_PPV_ARGS(&b->bounded));
  if(FAILED(boundedResult)){if(b->bounded)b->bounded->Release();b->bounded=nullptr;++failed;}
  replacement.CS=spatial==0?D3D12_SHADER_BYTECODE{kGISteadySpatial0,sizeof(kGISteadySpatial0)}:D3D12_SHADER_BYTECODE{kGISteadySpatial1,sizeof(kGISteadySpatial1)};
  const HRESULT steadyResult=create(device,&replacement,IID_PPV_ARGS(&b->steady));
  if(FAILED(steadyResult)){if(b->steady)b->steady->Release();b->steady=nullptr;++failed;}
  Log("GI_SPATIAL_R261 steady_ready=%u variant=%d",unsigned(b->steady!=nullptr),spatial);
  Log("GI_SPATIAL_R260 bounded_ready=%u variant=%d",unsigned(b->bounded!=nullptr),spatial);
  bundles[index].store(b,std::memory_order_release);++ready;
  Log("GI_SPATIAL_R259 ready=1 variant=%d second_history_weight=0 spatial_math_unchanged=1",spatial);return;
 }
 auto replacement=*desc;replacement.CachedPSO={};replacement.CS={kGIMotionHistoryShader,sizeof(kGIMotionHistoryShader)};
 const HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&b->adaptive));
 if(FAILED(hr)||!b->adaptive){if(b->adaptive)b->adaptive->Release();native->Release();delete b;++failed;
  Log("GI_PIPELINE_R217 ready=0 hr=0x%08lX native_fallback=1",static_cast<unsigned long>(hr));return;}
 replacement.CS={kGINoHistoryShader,sizeof(kGINoHistoryShader)};
 const HRESULT noHistoryResult=create(device,&replacement,IID_PPV_ARGS(&b->noHistory));
 if(FAILED(noHistoryResult)||!b->noHistory){
  if(b->noHistory)b->noHistory->Release();b->adaptive->Release();native->Release();delete b;++failed;
  Log("GI_PIPELINE_R217 ready=0 no_history_hr=0x%08lX native_fallback=1",static_cast<unsigned long>(noHistoryResult));return;
 }
 replacement.CS={kGIOutlierShader,sizeof(kGIOutlierShader)};
 const HRESULT outlierResult=create(device,&replacement,IID_PPV_ARGS(&b->outlier));
 if(FAILED(outlierResult)||!b->outlier){
  if(b->outlier)b->outlier->Release();b->noHistory->Release();b->adaptive->Release();native->Release();delete b;++failed;
  Log("GI_PIPELINE_R217 ready=0 outlier_hr=0x%08lX native_fallback=1",static_cast<unsigned long>(outlierResult));return;
 }
 replacement.CS={kGICleanupMaskShader,sizeof(kGICleanupMaskShader)};
 const HRESULT maskResult=create(device,&replacement,IID_PPV_ARGS(&b->mask));
 if(FAILED(maskResult)){if(b->mask)b->mask->Release();b->mask=nullptr;++failed;}
 replacement.CS={kGIZeroShader,sizeof(kGIZeroShader)};
 const HRESULT zeroResult=create(device,&replacement,IID_PPV_ARGS(&b->zero));
 if(FAILED(zeroResult)){if(b->zero)b->zero->Release();b->zero=nullptr;++failed;}
 replacement.CS={kGISmoothShader,sizeof(kGISmoothShader)};
 const HRESULT smoothResult=create(device,&replacement,IID_PPV_ARGS(&b->smooth));
 if(FAILED(smoothResult)){if(b->smooth)b->smooth->Release();b->smooth=nullptr;++failed;}
 Log("GI_SMOOTH_R217 ready=%u",unsigned(b->smooth!=nullptr));
 Log("GI_DIAGNOSTIC_R217 mask_ready=%u zero_ready=%u",unsigned(b->mask!=nullptr),unsigned(b->zero!=nullptr));
 bundles[index].store(b,std::memory_order_release);++ready;
 Log("GI_PIPELINE_R217 ready=1 native=%p adaptive=%p native_root_preserved=1 engine_record_copy=0",native,b->adaptive);
}
template<class Set,class DispatchFn> void Dispatch(ID3D12PipelineState* observed,unsigned count,bool& used,
 ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z,Set set,DispatchFn dispatch) {
 auto* b=Find(observed);
 auto* replacement=(b&&!b->spatial&&!spatialDispatch)?(historyDisabled?(outlierEnabled?b->smooth:b->noHistory):b->adaptive):nullptr;
 if(b&&!b->spatial&&!spatialDispatch&&historyDisabled&&outlierEnabled&&!replacement)replacement=b->noHistory;
 if(b&&!b->spatial&&!spatialDispatch&&historyDisabled&&activeView==DiagnosticView::Mask)replacement=b->mask;
 if(b&&!b->spatial&&!spatialDispatch&&historyDisabled&&activeView==DiagnosticView::Zero)replacement=b->zero;
 if(b&&b->spatial&&spatialDispatch)replacement=spatialSteady?b->steady:spatialHistoryBounded?b->bounded:b->adaptive;
 if(!replacement||count!=1){dispatch(list,x,y,z);return;}
 // Called only after DeviceUtil commits its native PSO and bindings.
 __try {set(list,replacement);dispatch(list,x,y,z);used=true;}
 __finally {set(list,observed);}
}
}
