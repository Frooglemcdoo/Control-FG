#pragma once
#include "pt_command_bindings.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <mutex>
#include <cstdint>
#include <vector>
#include <algorithm>
// One immutable frame/view output. Native draw recordings and their submitted
// leases must both retain this owner. Never recycle by frame count or resize an
// output still referenced by a recorded list. All GPU use is on the pinned queue.
namespace PTSurfaceComposition {
class Output {
    using MicrosoftResource=Microsoft::WRL::ComPtr<ID3D12Resource>;
    inline static std::atomic<std::uint64_t> residentBytes{0};
    static constexpr std::uint64_t MaximumBytes=4096ull*1024*1024;
    struct Storage {
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        Microsoft::WRL::ComPtr<IUnknown> queueIdentity;
        MicrosoftResource texture;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu,gpu;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> clearAllocator,armAllocator,producerAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> clearList,armList,producerList;
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        std::shared_ptr<void> dependencies;
        Storage* retiredNext=nullptr;
        std::uint64_t budget=0;
        ~Storage(){residentBytes.fetch_sub(budget);}
        UINT increment=0,width=0,height=0,stride=16;
        std::uint64_t nextFence=1,lastFence=0;
        bool reusable=false,linear=false;
        bool failed=false,invalidated=false,recorded=false,produced=false,inlineRecording=false;
    };
    inline static std::mutex retirementMutex;
    inline static Storage* retired=nullptr;
    inline static std::vector<std::unique_ptr<Storage>> idle;
    // Called only after all recording owners have released Output AND its last
    // GPU fence has completed. Keep allocations, never old scene dependencies.
    static void Reclaim(Storage* s){
        s->dependencies.reset();s->producerList.Reset();s->producerAllocator.Reset();
        if(s->reusable&&idle.size()<6){s->retiredNext=nullptr;idle.emplace_back(s);}
        else delete s;
    }
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
        auto& s=*storage;if(s.failed||s.nextFence==UINT64_MAX){s.failed=true;return false;}
        ID3D12CommandList* lists[]={list};s.queue->ExecuteCommandLists(1,lists);
        return Signal();
    }
    bool Signal(){
        auto& s=*storage;if(s.failed||s.nextFence==UINT64_MAX){s.failed=true;return false;}
        const auto value=s.nextFence++;
        if(FAILED(s.queue->Signal(s.fence.Get(),value))){s.failed=true;return false;}
        s.lastFence=value;return true;
    }
public:
    // Called under the renderer's submission serialization. Hook adapters must
    // suppress recursive observation of these private queue operations.
    static HRESULT Create(ID3D12CommandQueue* queue,UINT width,UINT height,std::shared_ptr<Output>& result,bool reusable=false,bool linear=false,UINT stride=16){
        PTCommandBindings::Mute privateRecording;
        result.reset();Poll();
        if(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT||!width||!height||
           width>16384||height>16384||(stride!=16&&stride!=32&&stride!=64)||(!linear&&stride!=16)||std::uint64_t(width)*height*stride>512ull*1024*1024)return E_INVALIDARG;
        auto pending=std::make_shared<Output>();
        if(reusable){
            Microsoft::WRL::ComPtr<IUnknown> identity;
            if(FAILED(queue->QueryInterface(IID_PPV_ARGS(&identity))))return E_INVALIDARG;
            {std::lock_guard<std::mutex> guard(retirementMutex);
                for(auto it=idle.begin();it!=idle.end();++it)if((*it)->queueIdentity.Get()==identity.Get()&&(*it)->width==width&&(*it)->height==height&&(*it)->linear==linear&&(*it)->stride==stride){
                    pending->storage=std::move(*it);idle.erase(it);break;}
                // A resolution/queue change must not strand the output budget.
                if(!pending->storage)idle.erase(std::remove_if(idle.begin(),idle.end(),[&](const auto& candidate){
                    return candidate->queueIdentity.Get()!=identity.Get()||candidate->width!=width||candidate->height!=height;
                }),idle.end());
            }
            if(pending->storage){auto& reused=*pending->storage;
                reused.invalidated=reused.recorded=reused.produced=reused.inlineRecording=false;
                if(!pending->Execute(reused.clearList.Get()))return E_FAIL;
                result=std::move(pending);return S_OK;
            }
        }
        pending->storage=std::make_unique<Storage>();auto& s=*pending->storage;
        const std::uint64_t bytes=std::uint64_t(width)*height*stride;
        // Retired storage may be evicted under pressure; never evict an output
        // held by a recording/history owner or with an incomplete GPU fence.
        if(residentBytes.load()>MaximumBytes-bytes){std::lock_guard<std::mutex> guard(retirementMutex);idle.clear();}
        auto used=residentBytes.load();
        do {if(used>MaximumBytes-bytes)return E_OUTOFMEMORY;}while(!residentBytes.compare_exchange_weak(used,used+bytes));
        s.budget=bytes;s.queue=queue;s.width=width;s.height=height;s.linear=linear;s.stride=stride;
        HRESULT hr=queue->QueryInterface(IID_PPV_ARGS(&s.queueIdentity));if(FAILED(hr))return hr;
        hr=queue->GetDevice(IID_PPV_ARGS(&s.device));if(FAILED(hr))return hr;
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;
        td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if(linear){td.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;td.Width=bytes;td.Height=1;td.Format=DXGI_FORMAT_UNKNOWN;td.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;}
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        hr=s.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,ReadState,nullptr,IID_PPV_ARGS(&s.texture));if(FAILED(hr))return hr;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;
        hr=s.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s.cpu));if(FAILED(hr))return hr;
        hd.NumDescriptors=1;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr=s.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s.gpu));if(FAILED(hr))return hr;
        s.increment=s.device->GetDescriptorHandleIncrementSize(hd.Type);
        auto handle=s.cpu->GetCPUDescriptorHandleForHeapStart();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=td.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        if(linear){uav.Format=DXGI_FORMAT_R32_TYPELESS;uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
            uav.Buffer.NumElements=static_cast<UINT>(bytes/4);uav.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;}
        s.device->CreateUnorderedAccessView(s.texture.Get(),nullptr,&uav,handle);
        s.device->CopyDescriptorsSimple(1,s.gpu->GetCPUDescriptorHandleForHeapStart(),handle,hd.Type);
        handle.ptr+=s.increment;
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=td.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        if(linear){srv.Format=DXGI_FORMAT_R32_TYPELESS;srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Buffer={};
            srv.Buffer.NumElements=static_cast<UINT>(bytes/4);srv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;}
        s.device->CreateShaderResourceView(s.texture.Get(),&srv,handle);
        hr=s.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s.fence));if(FAILED(hr))return hr;
        hr=s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&s.clearAllocator));if(FAILED(hr))return hr;
        hr=s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,s.clearAllocator.Get(),nullptr,IID_PPV_ARGS(&s.clearList));if(FAILED(hr))return hr;
        ID3D12DescriptorHeap* heaps[]={s.gpu.Get()};s.clearList->SetDescriptorHeaps(1,heaps);
        Transition(s.clearList.Get(),s.texture.Get(),ReadState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const float zero[4]{};
        if(linear){const UINT empty[4]{};s.clearList->ClearUnorderedAccessViewUint(s.gpu->GetGPUDescriptorHandleForHeapStart(),s.cpu->GetCPUDescriptorHandleForHeapStart(),s.texture.Get(),empty,0,nullptr);}
        else s.clearList->ClearUnorderedAccessViewFloat(s.gpu->GetGPUDescriptorHandleForHeapStart(),s.cpu->GetCPUDescriptorHandleForHeapStart(),s.texture.Get(),zero,0,nullptr);
        Transition(s.clearList.Get(),s.texture.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,ReadState);
        hr=s.clearList->Close();if(FAILED(hr))return hr;
        hr=s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&s.armAllocator));if(FAILED(hr))return hr;
        hr=s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,s.armAllocator.Get(),nullptr,IID_PPV_ARGS(&s.armList));if(FAILED(hr))return hr;
        s.armList->SetDescriptorHeaps(1,heaps);
        Transition(s.armList.Get(),s.texture.Get(),ReadState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const float armed[4]={0,0,0,1};
        if(linear){const UINT empty[4]{};s.armList->ClearUnorderedAccessViewUint(s.gpu->GetGPUDescriptorHandleForHeapStart(),s.cpu->GetCPUDescriptorHandleForHeapStart(),s.texture.Get(),empty,0,nullptr);}
        else s.armList->ClearUnorderedAccessViewFloat(s.gpu->GetGPUDescriptorHandleForHeapStart(),s.cpu->GetCPUDescriptorHandleForHeapStart(),s.texture.Get(),armed,0,nullptr);
        Transition(s.armList.Get(),s.texture.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,ReadState);
        hr=s.armList->Close();if(FAILED(hr))return hr;
        s.reusable=reusable;
        // Initialize every pixel on the GPU before any possible native draw.
        if(!pending->Execute(s.clearList.Get()))return E_FAIL;
        result=std::move(pending);return S_OK;
    }
    ID3D12Resource* Texture()const noexcept{return storage?storage->texture.Get():nullptr;}
    UINT Width()const noexcept{return storage?storage->width:0;}
    UINT64 Address()const noexcept{return storage&&storage->linear?storage->texture->GetGPUVirtualAddress():0;}
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
    // Encoder records between native lighting and its next barrier, preserving
    // roots/heaps/PSO. The encoder must test its GPU admission guard before
    // tracing and clear output alpha on every rejected pixel.
    bool RecordInline(ID3D12GraphicsCommandList* list,void* context,Encoder encode,std::shared_ptr<void> dependencies){
        PTCommandBindings::Mute privateRecording;
        std::lock_guard<std::mutex> guard(mutex);
        if(!storage||!list||!encode||!dependencies||storage->failed||storage->invalidated||storage->recorded||
           list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
        auto& s=*storage;s.dependencies=std::move(dependencies);s.recorded=true;s.inlineRecording=true;
        Transition(list,s.texture.Get(),ReadState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const bool success=encode(context,list,s.texture.Get(),Uav());
        Transition(list,s.texture.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,ReadState);
        if(!success)s.invalidated=true;return success;
    }
    bool Arm(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);
        if(!Queue(queue)||storage->linear||storage->failed||storage->invalidated||!storage->recorded||!storage->inlineRecording||storage->produced)return false;
        storage->produced=true;return Execute(storage->armList.Get());
    }
    // The separate admission buffer supplies GPU authorization for inline PT.
    // This changes lifecycle state only; no texture clear is legal on COPY.
    bool AdmitInline(){
        std::lock_guard<std::mutex> guard(mutex);
        if(!storage)return false;auto& s=*storage;
        if(s.failed||s.invalidated||!s.recorded||!s.inlineRecording||s.produced)return false;
        s.produced=true;return true;
    }
    // Extend retirement after every native submission, including replay.
    bool NativeSubmitted(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);
        if(!Queue(queue)){if(storage)storage->failed=true;return false;}return Signal();
    }
    bool Completed(){
        std::lock_guard<std::mutex> guard(mutex);if(!storage||storage->failed||!storage->lastFence)return false;
        const auto done=storage->fence->GetCompletedValue();return done!=UINT64_MAX&&done>=storage->lastFence;
    }
    static void Poll(){
        std::lock_guard<std::mutex> guard(retirementMutex);auto** at=&retired;
        while(*at){auto* s=*at;const auto done=s->fence?s->fence->GetCompletedValue():0;
            if(!s->failed&&done!=UINT64_MAX&&done>=s->lastFence){*at=s->retiredNext;Reclaim(s);}
            else at=&s->retiredNext;
        }
    }
    static void TrimIdle(){std::lock_guard<std::mutex> guard(retirementMutex);idle.clear();}
    static std::uint64_t ResidentBytes(){Poll();return residentBytes.load();}
    // Caller verifies the exact scene and current frame/view under the queue
    // submission lock immediately before this method. This API does not infer it.
    bool Produce(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);
        if(!Queue(queue)||storage->failed||storage->invalidated||!storage->recorded||storage->inlineRecording||storage->produced)return false;
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
            // Pending healthy objects retire on Poll; unknown fences quarantine
            // within the shared memory budget rather than silently freeing use.
            std::lock_guard<std::mutex> guard(retirementMutex);
            storage->retiredNext=retired;retired=storage.release();
        }else if(storage->reusable){
            std::lock_guard<std::mutex> guard(retirementMutex);Reclaim(storage.release());
        }
    }
};
}
