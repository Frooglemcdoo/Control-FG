#include "../src/rt_as_d3d12_ownership.h"
#include <dxgi1_6.h>
#include <cassert>
#include <atomic>
#include <iostream>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("D3D12 fixture failure");}
struct Marker final:IUnknown {
 std::atomic<ULONG> refs{1};std::atomic<unsigned>& destroyed;
 explicit Marker(std::atomic<unsigned>& n):destroyed(n){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id!=__uuidof(IUnknown))return E_NOINTERFACE;*out=this;AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n){++destroyed;delete this;}return n;}
};
static const GUID markerKey={0x58678645,0x1300,0x4343,{0x93,0x40,0x12,0x32,0x13,0x45,0x21,0x10}};
int main(){try{
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=65536;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ComPtr<ID3D12Resource> resource;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource)));
 std::atomic<unsigned> destroyed{0};auto* marker=new Marker(destroyed);Check(resource->SetPrivateDataInterface(markerKey,marker));marker->Release();
 rt_as_ownership::D3D12Ownership owners;auto& book=owners.Contracts();const auto key=owners.Register(resource.Get(),nullptr,{});resource.Reset();
 const auto recording=book.BeginList();book.Reference(recording,key);book.Close(recording);
 ComPtr<ID3D12Fence> gate,finished;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)));Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&finished)));
 Check(queue->Wait(gate.Get(),1));unsigned executed=0;
 auto execute=[&]{ID3D12CommandList* submitted[]={list.Get()};queue->ExecuteCommandLists(1,submitted);++executed;};
 owners.Execute(queue.Get(),{recording},execute);owners.Execute(queue.Get(),{recording},execute);
 book.Retire(key);book.Discard(recording);owners.Poll();assert(executed==2&&book.Pending()==2&&destroyed==0);
 Check(queue->Signal(finished.Get(),1));Check(gate->Signal(1));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);Check(finished->SetEventOnCompletion(1,event));
 const auto waited=WaitForSingleObject(event,10000);CloseHandle(event);assert(waited==WAIT_OBJECT_0);
 owners.Poll();assert(book.Pending()==0&&destroyed==1);
 std::cout<<"PASS actual WARP queue: blocked submission, replay, native retirement, recording discard, exact fence release; no AS builds or ray dispatches\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
