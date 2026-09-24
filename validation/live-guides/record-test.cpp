#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#define RR_SEH_TRY try
#define RR_SEH_EXCEPT(x) catch(...)
using UINT=unsigned;using DWORD=unsigned long;
constexpr UINT D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u,
 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE=64,D3D12_RESOURCE_STATE_UNORDERED_ACCESS=8;
struct ID3D12Resource {};
struct D3D12_RESOURCE_BARRIER {UINT Type=0;struct {ID3D12Resource* pResource=nullptr;UINT Subresource=0,StateBefore=0,StateAfter=0;} Transition;};
static unsigned seen=0,failAt=0;static unsigned long long* commandCount=nullptr;
static DWORD GetExceptionCode(){return 123;}
struct Heap {std::uint64_t GetGPUDescriptorHandleForHeapStart(){return 77;}};
static ID3D12Resource normal,specular,diffuse;static Heap heap;
static void step(){++seen;assert(*commandCount==seen);if(seen==failAt)throw std::runtime_error("simulated command fault");}
struct List {
 void ResourceBarrier(UINT n,D3D12_RESOURCE_BARRIER* b){step();assert(n==3);assert(b[0].Transition.pResource==&normal&&b[1].Transition.pResource==&diffuse&&b[2].Transition.pResource==&specular);for(UINT i=0;i<3;++i){assert(b[i].Transition.Subresource==~0u);assert(b[i].Transition.StateBefore==(seen==1?64u:8u));assert(b[i].Transition.StateAfter==(seen==1?8u:64u));}}
 void SetDescriptorHeaps(UINT n,Heap** h){step();assert(n==1&&*h==&heap);}
 void SetComputeRootSignature(void* p){step();assert(p==reinterpret_cast<void*>(1));}
 void SetPipelineState(void* p){step();assert(p==reinterpret_cast<void*>(2));}
 void SetComputeRoot32BitConstants(UINT n,UINT count,const void* p,UINT offset){step();assert(n==0&&count==4&&offset==0);const auto* x=static_cast<const UINT*>(p);assert(x[0]==1&&x[1]==3&&x[2]==2561&&x[3]==1441);}
 void SetComputeRootDescriptorTable(UINT n,std::uint64_t h){step();assert(n==1&&h==77);}
 void Dispatch(UINT x,UINT y,UINT z){step();assert(x==161&&y==91&&z==1);}
};
struct RRLiveOwner{UINT width=2561,height=1441;void* root=reinterpret_cast<void*>(1);void* pipeline=reinterpret_cast<void*>(2);};
struct RRLiveSlot{ID3D12Resource* normal;ID3D12Resource* specular;ID3D12Resource* diffuse;Heap* heap;};
struct RRGuideInputSnapshot {List* commandList;void* commandContext;};
#include "../../src/rr_live_record.h"
int main(){
 RRLiveOwner c;RRLiveSlot s{&normal,&specular,&diffuse,&heap};List list;
 struct Context{unsigned long long reserved,count;} context{};
 RRGuideInputSnapshot input{&list,&context};RRLiveConstants constants{1,3,2561,1441};
 for(unsigned fail=0;fail<=8u;++fail){seen=0;failAt=fail;context.count=0;DWORD fault=0;commandCount=&context.count;
  const bool result=RRLiveRecord(&c,&s,&input,&constants,&fault);
  assert(result==(fail==0));assert(seen==(fail?fail:8u));assert(context.count==seen);assert(fault==(fail?123u:0u));
 }
 puts("PASS: r21t live emitter; output-only barriers; RenoDX root constants/table order; ceil dispatch; all 8 command faults contained");
}
