#pragma once
// Included inside control_rr_clamp: shares its existing device/list hooks.
#include "../build/rr_gi_shaders.h"
struct GIBundle {ID3D12PipelineState* native=nullptr;ID3D12PipelineState* replacement=nullptr;unsigned kind=0;};
inline std::atomic<GIBundle*> giBundles[24]{};
inline std::atomic<unsigned> giBundleCount{0};
struct GIScope {ID3D12GraphicsCommandList* list=nullptr;ID3D12GraphicsCommandList* observedList=nullptr;ID3D12PipelineState* observed=nullptr;unsigned temporal=0,spatial=0,missed=0,passthrough=0;};
inline thread_local GIScope* giActive=nullptr;
inline int GITarget(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
 __try {
  if(!desc||!desc->pRootSignature||!desc->CS.pShaderBytecode||desc->CS.BytecodeLength<32||desc->CS.BytecodeLength>65536)return -1;
  const auto crc=CRC(desc->CS.pShaderBytecode,desc->CS.BytecodeLength);
  return crc==0x591fc46fu?0:crc==0x9018e4f2u?1:crc==0x2a6f7863u?2:-1;
 } __except(EXCEPTION_EXECUTE_HANDLER){return -1;}
}
inline void GICapture(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,void* result) noexcept {
 if(device!=hostDevice||!result)return;const int kind=GITarget(desc);if(kind<0)return;
 const unsigned index=giBundleCount.fetch_add(1,std::memory_order_acq_rel);if(index>=24)return;
 auto* b=new(std::nothrow) GIBundle{};if(!b)return;b->kind=unsigned(kind);
 if(FAILED(static_cast<IUnknown*>(result)->QueryInterface(IID_PPV_ARGS(&b->native)))||!b->native){delete b;return;}
 auto copy=*desc;copy.CachedPSO={};copy.CS={kRRGIShaders[kind].data,kRRGIShaders[kind].size};
 const HRESULT hr=createOriginal(device,&copy,IID_PPV_ARGS(&b->replacement));
 giBundles[index].store(b,std::memory_order_release);
 Log("RR_GI_CLONE_R16 kind=%u ready=%u hr=0x%08lX root_preserved=1",b->kind,unsigned(b->replacement!=nullptr),static_cast<unsigned long>(hr));
}
inline bool GIReady() noexcept {
 unsigned mask=0;for(auto& slot:giBundles){auto* b=slot.load(std::memory_order_acquire);if(b&&b->replacement)mask|=1u<<b->kind;}
 return mask==7&&listTable&&listTable[14]==reinterpret_cast<void*>(&Dispatch)&&listTable[25]==reinterpret_cast<void*>(&Set);
}
inline void GIObserveSet(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) noexcept {
 if(!giActive)return;
 giActive->observedList=list;giActive->observed=pso;
}
inline bool GIDispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z) {
 auto* scope=giActive;if(!scope)return false;
 if(scope->observedList!=list){++scope->missed;return false;}
 GIBundle* found=nullptr;
 for(auto& slot:giBundles){auto* b=slot.load(std::memory_order_acquire);if(b&&b->native==scope->observed){found=b;break;}}
 // The native filter also records auxiliary compute work. Only known GI
 // PSOs count toward the replacement contract; unrelated work stays native.
 if(!found){++scope->passthrough;return false;}
 if((scope->list&&scope->list!=list)||!found->replacement){++scope->missed;return false;}
 if(!scope->list)scope->list=list;
 __try {setOriginal(list,found->replacement);dispatchOriginal(list,x,y,z);
  if(found->kind==0)++scope->temporal;else ++scope->spatial;
 } __finally {setOriginal(list,scope->observed);}
 return true;
}
