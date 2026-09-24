// Executes the actual runtime header under host D3D12 mocks. Not a GPU/SEH test.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <new>
using DWORD=unsigned long;using UINT=unsigned;using UINT64=unsigned long long;using HRESULT=long;
struct LARGE_INTEGER {long long QuadPart=1000000;};static LARGE_INTEGER frequency{};
static DWORD lastError=13;static long long clockTicks=1000000;
static DWORD GetLastError(){return lastError;}static void SetLastError(DWORD v){lastError=v;}
static DWORD GetCurrentThreadId(){return 7;}
static void QueryPerformanceCounter(LARGE_INTEGER* p){p->QuadPart=clockTicks;}
static std::vector<std::string> logs;
template<class... T> static void Log(const char* f,T...){logs.push_back(f);}
#define IID_PPV_ARGS(p) p
#define SUCCEEDED(hr) ((hr)>=0)
#define FAILED(hr) ((hr)<0)
constexpr unsigned D3D12_QUERY_HEAP_TYPE_TIMESTAMP=1,D3D12_QUERY_TYPE_TIMESTAMP=2,D3D12_HEAP_TYPE_READBACK=3,D3D12_HEAP_FLAG_NONE=0,D3D12_RESOURCE_DIMENSION_BUFFER=1,D3D12_TEXTURE_LAYOUT_ROW_MAJOR=1,D3D12_RESOURCE_STATE_COPY_DEST=4,D3D12_FENCE_FLAG_NONE=0;
struct D3D12_QUERY_HEAP_DESC{unsigned Type=0,Count=0;};
struct D3D12_HEAP_PROPERTIES{unsigned Type=0,CreationNodeMask=0,VisibleNodeMask=0;};
struct D3D12_RESOURCE_DESC{unsigned Dimension=0;UINT64 Width=0;unsigned Height=0,DepthOrArraySize=0,MipLevels=0;struct{unsigned Count=0;}SampleDesc;unsigned Layout=0;};
struct D3D12_RANGE{std::size_t Begin,End;};
struct ID3D12QueryHeap{UINT64 values[512]{};bool initialized[512]{};};
struct ID3D12Resource{UINT64 values[512]{};HRESULT Map(unsigned,D3D12_RANGE*,void** p){*p=values;return 0;}};
struct ID3D12Fence{UINT64 completed=0,signal=0;UINT64 GetCompletedValue(){return completed;}};
struct ID3D12Device{
 ID3D12QueryHeap queries;ID3D12Resource readback;ID3D12Fence fence;
 HRESULT CreateQueryHeap(D3D12_QUERY_HEAP_DESC* d,ID3D12QueryHeap** p){assert(d->Count==512);*p=&queries;return 0;}
 HRESULT CreateCommittedResource(D3D12_HEAP_PROPERTIES*,unsigned,D3D12_RESOURCE_DESC* d,unsigned state,void*,ID3D12Resource** p){assert(d->Width==4096&&state==D3D12_RESOURCE_STATE_COPY_DEST);*p=&readback;return 0;}
 HRESULT CreateFence(UINT64,unsigned,ID3D12Fence** p){*p=&fence;return 0;}
};
static bool signalFail=false;
struct ID3D12CommandQueue{
 ID3D12Device device;unsigned signals=0;
 void AddRef(){}HRESULT GetDevice(ID3D12Device** p){*p=&device;return 0;}
 HRESULT GetTimestampFrequency(UINT64* p){*p=1000000;return 0;}
 HRESULT Signal(ID3D12Fence* p,UINT64 v){if(signalFail)return -1;p->signal=v;++signals;return 0;}
};
struct ID3D12GraphicsCommandList{
 unsigned ends=0,resolves=0;UINT64 ticks=1000;
 void EndQuery(ID3D12QueryHeap* heap,unsigned type,UINT i){assert(type==D3D12_QUERY_TYPE_TIMESTAMP);heap->initialized[i]=true;heap->values[i]=ticks;ticks+=1500;++ends;}
 void ResolveQueryData(ID3D12QueryHeap* heap,unsigned,UINT start,UINT count,ID3D12Resource* out,UINT64 offset){
  assert(count==2&&offset==start*8);for(unsigned i=0;i<count;++i){assert(heap->initialized[start+i]);out->values[start+i]=heap->values[start+i];}++resolves;
 }
};
struct RRGuideInputSnapshot{ID3D12GraphicsCommandList* commandList=nullptr;ID3D12CommandQueue* queue=nullptr;void* commandContext=nullptr;unsigned long long engineFrame=0,presentToken=0;};
static RRGuideInputSnapshot current{};
static bool RRGuideReadRendererContext(RRGuideInputSnapshot* p,const char**){*p=current;return true;}
struct PerfLeave{};
#include "performance-runtime.inc"
int main(){
 ID3D12CommandQueue queue;ID3D12GraphicsCommandList list,other;unsigned long long native[4]{};
 current={&list,&queue,native,60,10};RRPerfFrameMode(60,3840,2160,2560,1440,false,false,true,1);
 auto t=RRPerfBegin(current,RRPerfStage::NativeEvaluation);assert(t.list&&list.ends==1&&GetLastError()==13);
 RRPerfEnd(t);assert(list.ends==2&&list.resolves==1&&native[1]==3);
 RRPerfAfterPresent(11);assert(queue.signals==1&&rrPerf->pool.slots[t.lease.index].state==control_rr_perf::State::Submitted);
 RRPerfAfterPresent(12);assert(rrPerf->pool.slots[t.lease.index].state==control_rr_perf::State::Submitted);
 queue.device.fence.completed=1;RRPerfAfterPresent(13);assert(rrPerf->pool.slots[t.lease.index].state==control_rr_perf::State::Free);
 current.engineFrame=120;current.presentToken=13;RRPerfFrameMode(120,3840,2160,2560,1440,true,true,true,1);
 t=RRPerfBegin(current,RRPerfStage::ReflectionCopy);current.commandList=&other;RRPerfEnd(t);
 assert(list.resolves==1&&!rrPerf->pool.slots[t.lease.index].resolved);RRPerfAfterPresent(14);
 queue.device.fence.completed=2;RRPerfAfterPresent(15);assert(rrPerf->pool.slots[t.lease.index].state==control_rr_perf::State::Free);
 current.commandList=&list;current.engineFrame=180;current.presentToken=15;RRPerfFrameMode(180,3840,2160,3840,2160,true,true,true,1);
 t=RRPerfBegin(current,RRPerfStage::Distance);RRPerfEnd(t);signalFail=true;RRPerfAfterPresent(16);
 assert(rrPerfDisabled&&rrPerf->pool.slots[t.lease.index].state==control_rr_perf::State::Recorded);
 assert(!RRPerfBegin(current,RRPerfStage::LiveGuides).list&&GetLastError()==13);
 RRPerfReplayPrepared(180,2);RRPerfReplayJoined(180,3,400);RRPerfFilter(180,1);
 RRPerfEvaluation(180,true,true,true,RRPerfClock(),1,1,0,0);
 assert(rrPerfReplayPrepareMs==2&&rrPerfReplayWorkersMs==3&&rrPerfReplayBatches==400);
 delete rrPerf;rrPerf=nullptr;
 puts("PASS actual timestamp adapter: initialized pair only, delayed fence readback/reuse, list-change abandonment, native command count, LastError, signal failure disables profiling only");
}
