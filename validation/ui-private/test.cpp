// Execute the production correction against a minimal deterministic GPU mock.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_native_source_policy.h"
using UINT=unsigned;using UINT64=std::uint64_t;using DWORD=unsigned;using HRESULT=int;using HANDLE=void*;
constexpr HRESULT S_OK=0,E_FAIL=-1,E_INVALIDARG=-2,E_UNEXPECTED=-3,DXGI_ERROR_DEVICE_REMOVED=-4;
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)
#define IID_PPV_ARGS(p) 0,p
#define HRESULT_FROM_WIN32(x) (-int(x))
constexpr unsigned FALSE=0,WAIT_OBJECT_0=0,D3D12_COMMAND_LIST_TYPE_DIRECT=0,D3D12_FENCE_FLAG_NONE=0,D3D12_RESOURCE_DIMENSION_TEXTURE2D=2,D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u,D3D12_RESOURCE_STATE_COMMON=0,D3D12_RESOURCE_STATE_COPY_SOURCE=1,D3D12_RESOURCE_STATE_COPY_DEST=2;
struct Ref {int refs=1;void AddRef(){++refs;}void Release(){--refs;}};
struct Desc {unsigned Dimension=2,Width=1920,Height=1080,Format=24,DepthOrArraySize=1,MipLevels=1;struct {unsigned Count=1,Quality=0;} SampleDesc;};
struct ID3D12Resource:Ref {Desc desc;int pixels=0;unsigned state=0;Desc GetDesc(){return desc;}};
struct D3D12_RESOURCE_BARRIER {unsigned Type=0;struct {unsigned Subresource=0,StateBefore=0,StateAfter=0;ID3D12Resource* pResource=nullptr;} Transition;};
static bool timeout=false,signalFail=false,autoComplete=true;static unsigned copies=0,waits=0,executes=0;
struct ID3D12Fence;
struct ID3D12CommandAllocator:Ref {ID3D12Fence* owner=nullptr;UINT64 inFlight=0;HRESULT Reset();};
struct ID3D12CommandList:Ref {};
struct ID3D12GraphicsCommandList:ID3D12CommandList {
 ID3D12CommandAllocator* allocator=nullptr;
 HRESULT Close(){return S_OK;} HRESULT Reset(ID3D12CommandAllocator* a,void*){allocator=a;return S_OK;}
 void ResourceBarrier(unsigned n,D3D12_RESOURCE_BARRIER* a){for(unsigned i=0;i<n;++i){assert(a[i].Transition.pResource->state==a[i].Transition.StateBefore);a[i].Transition.pResource->state=a[i].Transition.StateAfter;}}
 void CopyResource(ID3D12Resource* dst,ID3D12Resource* src){assert(dst!=src);assert(dst->state==2&&src->state==1);dst->pixels=src->pixels;++copies;}
};
static ID3D12Fence* eventFence=nullptr;static UINT64 eventValue=0;
static ID3D12Fence* lastFence=nullptr;
struct ID3D12Fence:Ref {UINT64 done=0;HRESULT SetEventOnCompletion(UINT64 v,HANDLE){eventFence=this;eventValue=v;return S_OK;}UINT64 GetCompletedValue(){return done;}};
HRESULT ID3D12CommandAllocator::Reset(){assert(!owner||(!inFlight||(owner->done!=UINT64_MAX&&owner->done>=inFlight)));return S_OK;}
struct ID3D12Device:Ref {
 HRESULT CreateCommandAllocator(unsigned,int,ID3D12CommandAllocator** p){*p=new ID3D12CommandAllocator;return S_OK;}
 HRESULT CreateCommandList(unsigned,unsigned,ID3D12CommandAllocator*,void*,int,ID3D12GraphicsCommandList** p){*p=new ID3D12GraphicsCommandList;return S_OK;}
 HRESULT CreateFence(unsigned,unsigned,int,ID3D12Fence** p){*p=new ID3D12Fence;lastFence=*p;return S_OK;}
};
struct ID3D12CommandQueue:Ref {ID3D12GraphicsCommandList* submittedList=nullptr;ID3D12Device device;struct Desc {unsigned Type=0;};Desc GetDesc(){return {};}
 HRESULT GetDevice(int,ID3D12Device** p){*p=&device;device.AddRef();return S_OK;}
 void ExecuteCommandLists(unsigned,ID3D12CommandList** lists){++executes;submittedList=static_cast<ID3D12GraphicsCommandList*>(lists[0]);}
 HRESULT Signal(ID3D12Fence* f,UINT64 v){submittedList->allocator->owner=f;submittedList->allocator->inFlight=signalFail?UINT64_MAX:v;if(signalFail)return E_FAIL;if(autoComplete)f->done=v;return S_OK;}
};
static HANDLE CreateEventW(void*,unsigned,unsigned,void*){return reinterpret_cast<void*>(1);}
static DWORD WaitForSingleObject(HANDLE,unsigned){++waits;if(timeout)return 258;if(eventFence->done<eventValue)eventFence->done=eventValue;return WAIT_OBJECT_0;}
#include "../../src/fg_ui_work.h"
int main(){
 ID3D12Device dev; ID3D12CommandQueue q, replacement;
 autoComplete=false;
 FGUIWork slots[3];
 for(unsigned i=0;i<3;++i){assert(slots[i].Begin(&dev,100+i)==S_OK);assert(slots[i].Finish()==S_OK);assert(slots[i].Submit(&q,100+i)==S_OK);}
 assert(waits==0&&executes==3);
 // Allocator mock asserts completion before Reset; no per-submit CPU wait.
 assert(slots[0].Begin(&dev,103)==S_OK&&waits==1);
 assert(slots[0].Begin(&dev,103)==E_FAIL); // cannot reset an open recorder
 assert(slots[0].Finish()==S_OK);
 assert(slots[0].Begin(&dev,104)==E_FAIL); // cannot discard an unsubmitted job
 assert(slots[0].Submit(&q,103)==S_OK);
 unsigned before=executes;assert(slots[0].Submit(&q,103)==S_OK&&executes==before);
 slots[0].fence->done=slots[0].serial;before=waits;
 assert(slots[0].Begin(&dev,104)==S_OK&&waits==before);
 assert(slots[0].Finish()==S_OK&&slots[0].Submit(&replacement,104)==S_OK);
 {FGUIWork job;assert(job.Begin(&dev,1)==S_OK&&job.Finish()==S_OK);before=executes;assert(FAILED(job.Submit(&q,2)));assert(executes==before);assert(FAILED(job.Begin(&dev,3)));}
 {FGUIWork job;assert(job.Begin(&dev,1)==S_OK&&job.Finish()==S_OK);signalFail=true;assert(FAILED(job.Submit(&q,1)));signalFail=false;assert(FAILED(job.Begin(&dev,2)));}
 {FGUIWork job;assert(job.Begin(&dev,1)==S_OK&&job.Finish()==S_OK&&job.Submit(&q,1)==S_OK);timeout=true;assert(FAILED(job.Begin(&dev,2)));timeout=false;assert(FAILED(job.Begin(&dev,3)));}
 {FGUIWork job;assert(job.Begin(&dev,1)==S_OK&&job.Finish()==S_OK&&job.Submit(&q,1)==S_OK);job.fence->done=UINT64_MAX;assert(FAILED(job.Begin(&dev,2)));}
 {FGUIWork job;assert(job.Begin(&dev,1)==S_OK&&job.Finish()==S_OK);assert(FAILED(job.Submit(nullptr,1)));}
 puts("PASS UI private work: three in-flight frames, retirement before reset, no immediate wait, open/pending guards, idempotent submit, queue change, stale-frame rejection, signal/timeout/device-removal quarantine");
}
