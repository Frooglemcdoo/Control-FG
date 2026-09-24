// Execute the production correction against a minimal deterministic GPU mock.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../src/fg_native_source_policy.h"
using UINT=unsigned;using UINT64=std::uint64_t;using DWORD=unsigned;using HRESULT=int;using HANDLE=void*;
constexpr HRESULT S_OK=0,E_FAIL=-1,E_INVALIDARG=-2;
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
struct DXGI_SWAP_CHAIN_DESC1 {UINT BufferCount=2;};
struct IDXGISwapChain3 {ID3D12Resource buffers[2];UINT index=0,count=2;bool queryFail=false;
 HRESULT GetDesc1(DXGI_SWAP_CHAIN_DESC1* d){d->BufferCount=count;return S_OK;}
 UINT GetCurrentBackBufferIndex(){return index;}
 HRESULT GetBuffer(UINT n,int,ID3D12Resource** p){if(queryFail)return E_FAIL;*p=&buffers[n];(*p)->AddRef();return S_OK;}
};
static ID3D12CommandQueue testDirectQueue;
static ID3D12CommandQueue* activeTestQueue=&testDirectQueue;
static ID3D12CommandQueue* GetHdr10BridgeDirectQueue(){return activeTestQueue;}
static DWORD lastError=42;
static DWORD GetLastError(){return lastError;}static void SetLastError(DWORD v){lastError=v;}
static HANDLE CreateEventW(void*,unsigned,unsigned,void*){return reinterpret_cast<void*>(1);}
static void CloseHandle(HANDLE){}static DWORD WaitForSingleObject(HANDLE,unsigned){++waits;if(timeout)return 258;if(eventFence->done<eventValue)eventFence->done=eventValue;return WAIT_OBJECT_0;}
static bool FGAlignActive(unsigned long long){return true;}static bool FGPixelNeedsPresent(unsigned long long){return false;}
static void Log(const char*,...){}
static bool matched=true;static unsigned nativeIndex=0;
static control_fg_native_source::Result FGNativeSourceSelect(const void*,ID3D12Resource* const* resources,UINT count,UINT fallback){
 control_fg_native_source::Result r;r.source=fallback;r.status=control_fg_native_source::Status::NoResourceMatch;
 if(matched&&count==2&&resources[0]&&resources[1]){r.source=nativeIndex;r.nativeIndex=nativeIndex;r.status=control_fg_native_source::Status::Matched;}return r;
}
#include "../../src/fg_sdr_correction.h"
int main(){
 IDXGISwapChain3 chain;chain.buffers[0].pixels=10;chain.buffers[1].pixels=20;
 {FGSDRCorrection fix;
 assert(fix.Apply(&chain,&chain,1)==S_OK);assert(copies==0&&waits==0);
 chain.index=1;assert(fix.Apply(&chain,&chain,2)==S_OK);assert(copies==1&&waits==0&&chain.buffers[1].pixels==10);
 assert(chain.buffers[0].state==0&&chain.buffers[1].state==0);
 // Poll retirement even when the next frame requires no correction.
 nativeIndex=1;assert(fix.Apply(&chain,&chain,3)==S_OK);assert(chain.buffers[0].refs==1&&chain.buffers[1].refs==1);
 chain.index=0;chain.buffers[1].pixels=30;assert(fix.Apply(&chain,&chain,4)==S_OK);assert(chain.buffers[0].pixels==30&&copies==2);
 matched=false;assert(fix.Apply(&chain,&chain,5)==S_OK&&copies==2);matched=true;
 chain.count=3;assert(fix.Apply(&chain,&chain,6)==S_OK&&copies==2);chain.count=2;
 chain.queryFail=true;assert(fix.Apply(&chain,&chain,7)==S_OK&&copies==2);chain.queryFail=false;
 assert(lastError==42);assert(fix.PrepareResize(8,"test")==S_OK);assert(waits==0);
 assert(chain.buffers[0].refs==1&&chain.buffers[1].refs==1);
 // Recreate after resize using replacement resources, no stale references.
 IDXGISwapChain3 resized;assert(fix.Apply(&resized,&resized,9)==S_OK);assert(fix.PrepareResize(10,"test")==S_OK);
 }
 assert(testDirectQueue.refs==1&&testDirectQueue.device.refs==1);
 autoComplete=false;
 {FGSDRCorrection fix;unsigned before=waits;
 for(unsigned f=0;f<3;++f)assert(fix.Apply(&chain,&chain,20+f)==S_OK);
 assert(waits==before);assert(chain.buffers[0].refs==4&&chain.buffers[1].refs==4);
 // Fourth copy must wait for its allocator; the mock rejects early Reset.
 assert(fix.Apply(&chain,&chain,23)==S_OK);assert(waits==before+1);
 assert(fix.PrepareResize(24,"test")==S_OK);assert(chain.buffers[0].refs==1&&chain.buffers[1].refs==1);
 }
 {FGSDRCorrection fix;assert(fix.Apply(&chain,&chain,30)==S_OK);unsigned before=waits;fix.Shutdown();assert(waits==before+1);fix.Shutdown();assert(waits==before+1);assert(chain.buffers[0].refs==1);}
 {FGSDRCorrection fix;chain.buffers[1].desc.Width=100;unsigned before=copies;assert(FAILED(fix.Apply(&chain,&chain,40)));assert(copies==before);chain.buffers[1].desc.Width=1920;assert(FAILED(fix.Apply(&chain,&chain,41)));}
 {ID3D12CommandQueue replacement;FGSDRCorrection fix;assert(fix.Apply(&chain,&chain,42)==S_OK);activeTestQueue=&replacement;assert(fix.Apply(&chain,&chain,43)==S_OK);assert(testDirectQueue.refs==1);fix.Shutdown();assert(replacement.refs==1&&chain.buffers[0].refs==1);activeTestQueue=&testDirectQueue;}
 {IDXGISwapChain3 bad;FGSDRCorrection fix;for(unsigned f=0;f<3;++f)assert(fix.Apply(&bad,&bad,44+f)==S_OK);timeout=true;unsigned before=executes;assert(FAILED(fix.Apply(&bad,&bad,47)));assert(executes==before);assert(bad.buffers[0].refs==4);fix.Shutdown();timeout=false;}
 // Each failure scenario uses resources whose test lifetime exceeds the context.
 {IDXGISwapChain3 bad;FGSDRCorrection fix;assert(fix.Apply(&bad,&bad,50)==S_OK);timeout=true;assert(FAILED(fix.PrepareResize(51,"test")));assert(bad.buffers[0].refs==2);unsigned before=executes;assert(FAILED(fix.Apply(&bad,&bad,52)));assert(executes==before);fix.Shutdown();assert(bad.buffers[0].refs==2);timeout=false;}
 {IDXGISwapChain3 bad;FGSDRCorrection fix;signalFail=true;assert(FAILED(fix.Apply(&bad,&bad,60)));assert(bad.buffers[0].refs==2);assert(FAILED(fix.PrepareResize(61,"test")));fix.Shutdown();signalFail=false;}
 {IDXGISwapChain3 bad;FGSDRCorrection fix;assert(fix.Apply(&bad,&bad,70)==S_OK);lastFence->done=UINT64_MAX;assert(FAILED(fix.Apply(&bad,&bad,71)));assert(bad.buffers[0].refs==2);fix.Shutdown();}
 puts("PASS async SDR: both directions, zero immediate waits, completed retirement, three in-flight slots, safe allocator reuse, resize/replacement, shutdown, guards, timeout/signal/device-removal quarantine");
}
