#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include "../src/gi_reuse/d3d12_snapshot_retirement.h"
using Microsoft::WRL::ComPtr;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT failure");}
void Require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
unsigned destroyed=0;
struct Owner {ComPtr<ID3D12Resource> read;~Owner(){++destroyed;}};
using Runtime=gi_reuse::D3D12SnapshotRetirement<Owner,8,2,4096>;
HRESULT FailSignal(ID3D12CommandQueue*,ID3D12Fence*,UINT64){return E_FAIL;}
void Drain(ID3D12Device* d,ID3D12CommandQueue* q){
 ComPtr<ID3D12Fence> f;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&f)));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");
 Check(q->Signal(f.Get(),1));Check(f->SetEventOnCompletion(1,event));
 const auto result=WaitForSingleObject(event,30000);CloseHandle(event);Require(result==WAIT_OBJECT_0,"timeout");
}
int main() try {
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
 ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d)));
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> q;Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&q)));
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=256;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 auto owner=std::make_unique<Owner>();Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&owner->read)));
 ComPtr<ID3D12CommandAllocator> a,b;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&a)));Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&b)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,a.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
 Runtime runtime;Runtime::Recording record{runtime.Admit(1,256,owner),1};Require(record.id && !owner,"admission");
 ID3D12CommandList* lists[]={list.Get()};auto flight=runtime.Before(q.Get(),&record,1);Require(flight.count==1,"ticket");
 q->ExecuteCommandLists(1,lists);Require(runtime.After(flight),"signal");Drain(d.Get(),q.Get());runtime.Poll();
 Require(!runtime.View(record.id),"completed closed list must remain unpublished");
 // Replay only after previous GPU execution completes (D3D12 API contract).
 ComPtr<ID3D12Fence> gate;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)));Check(q->Wait(gate.Get(),1));
 flight=runtime.Before(q.Get(),&record,1);q->ExecuteCommandLists(1,lists);Require(runtime.After(flight),"replay signal");
 Check(list->Reset(b.Get(),nullptr));Require(runtime.Seal(record),"successful reset seals old recording");
 runtime.Poll();Require(!runtime.View(record.id),"reset must not publish pending GPU work");
 std::unique_ptr<Owner> retired;Require(!runtime.Take(record.id,retired)&&destroyed==0,"pending owner retained");
 Check(gate->Signal(1));Drain(d.Get(),q.Get());runtime.Poll();Require(runtime.View(record.id)!=nullptr,"completed sealed recording published");
 Require(runtime.Take(record.id,retired)&&destroyed==0,"detach without COM destruction");retired.reset();Require(destroyed==1&&runtime.RetainedBytes()==0,"release outside host lock");Check(list->Close());
 auto failedOwner=std::make_unique<Owner>();Runtime::Recording failed{runtime.Admit(2,1,failedOwner),2};
 flight=runtime.Before(q.Get(),&failed,1);Require(!runtime.After(flight,&FailSignal),"failure propagated");runtime.Seal(failed);runtime.Poll();
 Require(runtime.Quarantined(failed.id)&&!runtime.Take(failed.id,retired),"failed signal pins ownership");
 // No GPU operation was submitted for failedOwner; all earlier work is drained.
 runtime.DeviceStopped();Require(destroyed==2,"explicit stopped-device cleanup");
 ComPtr<ID3D12InfoQueue> messages;Check(d.As(&messages));
 for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T n=0;Check(messages->GetMessage(i,nullptr,&n));std::vector<unsigned char> bytes(n);auto* m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(messages->GetMessage(i,m,&n));Require(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR,m->pDescription);}
 std::puts("PASS persistent snapshot fences: actual WARP Execute, completed replay, reset while GPU blocked, delayed publication, deferred release, failed-signal quarantine, debug layer clean");
} catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
