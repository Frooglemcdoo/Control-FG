#pragma once
#include "rr_suite_policy.h"
#ifdef CONTROL_RR_F9_SUITE
#include "../build/spec_suite.generated.h"
namespace rr_suite {
// Bounded immutable PSOs held until process exit, including their identity PSO.
struct Bundle {ID3D12PipelineState* native=nullptr;ID3D12PipelineState* variants[6]{};};
inline std::atomic<Bundle*> bundles[8]{};
inline std::atomic<unsigned> allocated{0},ready{0},failures{0};
inline thread_local ID3D12GraphicsCommandList* observedList=nullptr;
inline thread_local ID3D12PipelineState* observedPso=nullptr;
inline thread_local unsigned long long observedFrame=0;
template<class Create> void Capture(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,void* native,Create create) noexcept {
 const auto n=allocated.fetch_add(1);if(n>=8){++failures;return;}
 auto* b=new(std::nothrow) Bundle{};if(!b){++failures;return;}
 bool okay=SUCCEEDED(static_cast<IUnknown*>(native)->QueryInterface(IID_PPV_ARGS(&b->native)));
 for(unsigned i=0;i<6&&okay;++i){auto d=desc;d.CachedPSO={};d.CS=kSpecSuiteShaders[i];okay=SUCCEEDED(create(device,&d,IID_PPV_ARGS(&b->variants[i])))&&b->variants[i];}
 if(!okay){for(auto* p:b->variants)if(p)p->Release();if(b->native)b->native->Release();delete b;++failures;return;}
 bundles[n].store(b,std::memory_order_release);++ready;
}
inline void Observe(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) noexcept {
 observedList=list;observedPso=pso;observedFrame=frame.load();
}
template<class Set,class Dispatch> bool Run(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z,Set set,Dispatch dispatch) {
 const auto n=latched.load();const auto f=frame.load();
 if(!n||n>count||observedList!=list||observedFrame!=f)return false;
 Bundle* found=nullptr;for(auto& slot:bundles){auto* b=slot.load(std::memory_order_acquire);if(b&&b->native==observedPso){found=b;break;}}
 if(!found)return false;
 __try {set(list,found->variants[steps[n-1].variant]);dispatch(list,x,y,z);
  dispatchStep.store(n);dispatchFrame.store(f);++dispatches;
 } __finally {set(list,observedPso);}
 return true;
}
}
#endif
