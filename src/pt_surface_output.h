#pragma once
#include "pt_command_bindings.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <mutex>
#include <cstdint>
// One immutable frame/view output. Native draw recordings and their submitted
// leases must both retain this owner. Never recycle by frame count or resize an
// output still referenced by a recorded list. All GPU use is on the pinned queue.
namespace PTSurfaceComposition {
class Output {
    using MicrosoftResource=Microsoft::WRL::ComPtr<ID3D12Resource>;
    struct Storage {
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        Microsoft::WRL::ComPtr<IUnknown> queueIdentity;
        MicrosoftResource texture;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu,gpu;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> clearAllocator,producerAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> clearList,producerList;
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        std::shared_ptr<void> dependencies;
        UINT increment=0,width=0,height=0;
        std::uint64_t nextFence=1,lastFence=0;
        bool failed=false,invalidated=false,recorded=false,produced=false;
    };
    std::unique_ptr<Storage> storage;
    std::mutex mutex;
    static constexpr D3D12_RESOURCE_STATES ReadState=static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    static void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* texture,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={texture,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);
    }
    bool Queue(ID3D12CommandQueue* q)const {
        if(!storage||!q)return false;Microsoft::WRL::ComPtr<IUnknown> key;
        return SUCCEEDED(q->QueryInterface(IID_PPV_ARGS(&key)))&&key.Get()==storage->queueIdentity.Get();
    }
    bool Execute(ID3D12GraphicsCommandList* list){
        auto& s=*storage;if(s.failed||s.nextFence==UINT64_MAX)return false;
        ID3D12CommandList* lists[]={list};s.queue->ExecuteCommandLists(1,lists);
        const auto value=s.nextFence++;
        if(FAILED(s.queue->Signal(s.fence.Get(),value))){s.failed=true;return false;}
        s.lastFence=value;return true;
    }
public:
    // Called under the renderer's submission serialization. Hook adapters must
    // suppress recursive observation of these private queue operations.
    static HRESULT Create(ID3D12CommandQueue* queue,UINT width,UINT height,std::shared_ptr<Output>& result){
        PTCommandBindings::Mute privateRecording;
        result.reset();
        if(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT||!width||!height||
           width>16384||height>16384||std::uint64_t(width)*height*16>256ull*1024*1024)return E_INVALIDARG;
        auto pending=std::make_shared<Output>();pending->storage=std::make_unique<Storage>();auto& s=*pending->storage;
        s.queue=queue;s.width=width;s.height=height;
        HRESULT hr=queue->QueryInterface(IID_PPV_ARGS(&s.queueIdentity));if(FAILED(hr))return hr;
        hr=queue->GetDevice(IID_PPV_ARGS(&s.device));if(FAILED(hr))return hr;
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;
        td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        hr=s.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,ReadState,nullptr,IID_PPV_ARGS(&s.texture));if(FAILED(hr))return hr;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;
        hr=s.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s.cpu));if(FAILED(hr))return hr;
        hd.NumDescriptors=1;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr=s.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s.gpu));if(FAILED(hr))return hr;
        s.increment=s.device->GetDescriptorHandleIncrementSize(hd.Type);
        auto handle=s.cpu->GetCPUDescriptorHandleForHeapStart();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=td.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        s.device->CreateUnorderedAccessView(s.texture.Get(),nullptr,&uav,handle);
        s.device->CopyDescriptorsSimple(1,s.gpu->GetCPUDescriptorHandleForHeapStart(),handle,hd.Type);
        handle.ptr+=s.increment;
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=td.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        s.device->CreateShaderResourceView(s.texture.Get(),&srv,handle);
        hr=s.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s.fence));if(FAILED(hr))return hr;
        hr=s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&s.clearAllocator));if(FAILED(hr))return hr;
        hr=s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,s.clearAllocator.Get(),nullptr,IID_PPV_ARGS(&s.clearList));if(FAILED(hr))return hr;
        ID3D12DescriptorHeap* heaps[]={s.gpu.Get()};s.clearList->SetDescriptorHeaps(1,heaps);
        Transition(s.clearList.Get(),s.texture.Get(),ReadState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const float zero[4]{};
        s.clearList->ClearUnorderedAccessViewFloat(s.gpu->GetGPUDescriptorHandleForHeapStart(),s.cpu->GetCPUDescriptorHandleForHeapStart(),s.texture.Get(),zero,0,nullptr);
        Transition(s.clearList.Get(),s.texture.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,ReadState);
        hr=s.clearList->Close();if(FAILED(hr))return hr;
        // Initialize every pixel on the GPU before any possible native draw.
        if(!pending->Execute(s.clearList.Get()))return E_FAIL;
        result=std::move(pending);return S_OK;
    }
    ID3D12Resource* Texture()const noexcept{return storage?storage->texture.Get():nullptr;}
    D3D12_CPU_DESCRIPTOR_HANDLE Srv()const noexcept{
        if(!storage)return {};auto h=storage->cpu->GetCPUDescriptorHandleForHeapStart();h.ptr+=storage->increment;return h;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE Uav()const noexcept{return storage?storage->cpu->GetCPUDescriptorHandleForHeapStart():D3D12_CPU_DESCRIPTOR_HANDLE{};}
    // Encoder binds its own roots/heaps/pipeline and writes float4(reflected
    // radiance, 0/1 admission). Dependencies own its SBT, scene lease and bindings.
    // Failed recording is discarded without executing partial GPU commands.
    using Encoder=bool(*)(void*,ID3D12GraphicsCommandList*,ID3D12Resource*,D3D12_CPU_DESCRIPTOR_HANDLE);
    bool Record(void* context,Encoder encode,std::shared_ptr<void> dependencies){
        PTCommandBindings::Mute privateRecording;
        std::lock_guard<std::mutex> guard(mutex);
        if(!storage||!encode||!dependencies||storage->failed||storage->invalidated||storage->recorded)return false;
        auto& s=*storage;Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
        if(FAILED(s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)))||
           FAILED(s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))))return false;
        Transition(list.Get(),s.texture.Get(),ReadState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        if(!encode(context,list.Get(),s.texture.Get(),Uav()))return false;
        Transition(list.Get(),s.texture.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,ReadState);
        if(FAILED(list->Close()))return false;
        s.producerAllocator=std::move(allocator);s.producerList=std::move(list);s.dependencies=std::move(dependencies);s.recorded=true;return true;
    }
    // Caller verifies the exact scene and current frame/view under the queue
    // submission lock immediately before this method. This API does not infer it.
    bool Produce(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);
        if(!Queue(queue)||storage->failed||storage->invalidated||!storage->recorded||storage->produced)return false;
        storage->produced=true;return Execute(storage->producerList.Get());
    }
    // Before replaying a consumer list, clear in the SAME queue. Earlier reads
    // finish first; later reads select native radiance. No upload flag is edited.
    // Invalidated output can never receive a late producer submission.
    bool Invalidate(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);
        if(!Queue(queue)||storage->failed)return false;
        if(storage->invalidated)return true;
        storage->invalidated=true;return Execute(storage->clearList.Get());
    }
    ~Output(){
        if(!storage)return;
        const auto completed=storage->fence?storage->fence->GetCompletedValue():0;
        if(storage->failed||completed==UINT64_MAX||completed<storage->lastFence){
            // Never free GPU-referenced texture, lists, heaps or scene resources
            // after a missing/failed retirement proof (including device removal).
            (void)storage.release();
        }
    }
};
}
