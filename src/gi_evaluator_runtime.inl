// Included inside native_lighting. Native PSOs own alternates through COM private data.
namespace gi_evaluator {
inline const GUID kVariant={0x309e0001,0x1739,0x4401,{0x91,0x33,0x41,0x51,0x61,0x71,0x81,0x91}};
inline const GUID kMode={0x309e0002,0x1739,0x4401,{0x91,0x33,0x41,0x51,0x61,0x71,0x81,0x91}};
inline bool Enabled() noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 static const bool value=[] {char v[8]{};return gi_reuse::SingleProducerEnabled()||(GetEnvironmentVariableA("CONTROLTOH_GI_EVALUATOR",v,sizeof(v))==1&&v[0]=='1');}();return value;
#else
 return false;
#endif
}
inline std::atomic<bool> ready{false};
inline auto& requested=gi_reuse::ownedRequested;
inline std::atomic<unsigned long long> revision{0},lastReported{~0ull};
#ifdef CONTROL_GI_LONG_SEQUENCE
#include "../build/gi_evaluator.generated.h"
#endif
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native,CreateCompute create) noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 if((!Enabled()&&!GIOwnedPipeline::Enabled())||!device||!desc||!native||!create||!desc->CS.pShaderBytecode||desc->CS.BytecodeLength!=kNativeEvaluatorSize)return;
 char sha[65]{};
 if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,sha)||strcmp(sha,"1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de"))return;
 const auto owned=GIOwnedPipeline::Attach(device,desc->pRootSignature,native);
 if(GIOwnedPipeline::Enabled())Trace("GI_OWNED_PIPELINE attached=%u hr=0x%08lX dispatch_hook=1",unsigned(owned==S_OK),static_cast<unsigned long>(owned));
 if(!Enabled())return;
 auto replacement=*desc;replacement.CS={kEvaluatorShader,sizeof(kEvaluatorShader)};replacement.CachedPSO={};
 ID3D12PipelineState* alternate=nullptr;
 HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&alternate));
 if(SUCCEEDED(hr)&&!alternate)hr=E_UNEXPECTED;
 if(SUCCEEDED(hr)){const UINT mode=2;hr=alternate->SetPrivateData(kMode,sizeof(mode),&mode);}
 if(SUCCEEDED(hr)){const UINT mode=1;ID3D12PipelineState* owner=nullptr;hr=native->QueryInterface(IID_PPV_ARGS(&owner));
  // Query only the native owner interface; never retain a global device/PSO.
  if(SUCCEEDED(hr)&&owner){hr=owner->SetPrivateData(kMode,sizeof(mode),&mode);
   if(SUCCEEDED(hr))hr=owner->SetPrivateDataInterface(kVariant,alternate);owner->Release();}
 }
 if(SUCCEEDED(hr)&&alternate)GIOwnedPipeline::Share(native,alternate);
 if(alternate)alternate->Release();
 if(SUCCEEDED(hr)){ready.store(true);if(gi_reuse::SingleProducerEnabled())gi_reuse::singleEvaluatorReady=true;}
 Trace("GI_EVALUATOR_R309 ready=%u hr=0x%08lX shader=gi6 native_fallback=%u",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr),unsigned(FAILED(hr)));
#else
 (void)device;(void)desc;(void)native;(void)create;
#endif
}
inline ID3D12PipelineState* Select(ID3D12PipelineState* native) noexcept {
 if(!Enabled()||!requested.load()||!native)return nullptr;
 ID3D12PipelineState* result=nullptr;UINT size=sizeof(result);
 if(FAILED(native->GetPrivateData(kVariant,&size,&result)))return nullptr;
 if(size!=sizeof(result)){if(result)result->Release();return nullptr;}return result;
}
inline void Bind(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) noexcept {
 if(!Enabled())return;UINT mode=0,size=sizeof(mode);
 if(!pso||FAILED(pso->GetPrivateData(kMode,&size,&mode))||size!=sizeof(mode))mode=0;
 list->SetPrivateData(kMode,sizeof(mode),&mode);
}
inline void Dispatch(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled())return;UINT mode=0,size=sizeof(mode);
 if(FAILED(list->GetPrivateData(kMode,&size,&mode))||size!=sizeof(mode)||!mode)return;
 settings_check::Record(mode==2?settings_check::OwnedGI:settings_check::NativeGI);
 if(mode==2&&gi_reuse::SingleProducerEnabled())++gi_reuse::singleEvaluatorDispatches;
 const auto rev=revision.load();const auto key=rev*4+mode;
 if(lastReported.exchange(key)!=key)Trace("GI_EVALUATOR_R309 dispatch_recorded=1 mode=%s revision=%llu gpu_completion=0",mode==2?"translated":"native",rev);
}
inline void Toggle() noexcept {if(!ready.load()&&!gi_reuse::SingleProducerEnabled())return;GIOwnedPipeline::active=false;const bool value=!requested.load();gi_reuse::singleRayApplied=false;requested.store(value);const auto rev=++revision;Trace("GI_EVALUATOR_R309 requested=%s revision=%llu",value?"translated":"native",rev);}
inline const wchar_t* Label() noexcept {
 if(gi_reuse::SingleProducerEnabled()){
  if(!requested.load())return L"GI: NATIVE";
  return gi_reuse::singleRayApplied.load()&&gi_reuse::singleEvaluatorDispatches.load()?L"GI: OWNED":L"GI: OWNED WAIT";
 }
 if(GIOwnedPipeline::Enabled()&&ready.load())return !requested.load()?L"GI: NATIVE":GIOwnedPipeline::active.load()?L"GI: OWNED":L"GI: OWNED WAIT";return !ready.load()?L"GI EVAL: WAIT":requested.load()?L"GI EVAL: TRANSLATED":L"GI EVAL: NATIVE";}
}
