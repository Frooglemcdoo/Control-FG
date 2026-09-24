#pragma once
// Submit copies on the presentation direct queue without an immediate CPU wait.
// Each slot owns its allocator, list and buffer references until its fence retires.
class FGSDRCorrection {
    struct Slot {
        ID3D12CommandAllocator* allocator=nullptr;
        ID3D12GraphicsCommandList* list=nullptr;
        ID3D12Resource* source=nullptr;
        ID3D12Resource* destination=nullptr;
        UINT64 value=0;
    } slots_[3]{};
    ID3D12Device* device_=nullptr;
    ID3D12CommandQueue* queue_=nullptr;
    ID3D12Fence* fence_=nullptr;
    HANDLE event_=nullptr;
    UINT64 serial_=0,submitted_=0,retired_=0,reuseWaits_=0,drainWaits_=0;
    unsigned next_=0;
    bool failed_=false,copyWait_=false,shutdown_=false;

    HRESULT Retire(Slot& slot,bool wait,bool draining) noexcept {
        if(!slot.source)return S_OK;
        // A failed queue Signal cannot prove completion. Keep all objects alive.
        if(slot.value==UINT64_MAX)return E_FAIL;
        UINT64 done=fence_->GetCompletedValue();
        if(done==UINT64_MAX)return E_FAIL;
        if(done<slot.value){
            if(!wait)return S_OK;
            if(draining)++drainWaits_;else{++reuseWaits_;copyWait_=true;}
            HRESULT hr=fence_->SetEventOnCompletion(slot.value,event_);
            if(FAILED(hr))return hr;
            const DWORD result=WaitForSingleObject(event_,5000);
            done=fence_->GetCompletedValue();
            if(result!=WAIT_OBJECT_0||done==UINT64_MAX||done<slot.value)return E_FAIL;
        }
        slot.source->Release();slot.destination->Release();
        slot.source=nullptr;slot.destination=nullptr;slot.value=0;++retired_;
        return S_OK;
    }
    HRESULT Drain() noexcept {
        for(auto& slot:slots_){const HRESULT hr=Retire(slot,true,true);if(FAILED(hr)){failed_=true;return hr;}}
        return S_OK;
    }
    void Release() noexcept {
        for(const auto& slot:slots_)if(slot.source)return; // Quarantine in-flight state.
        for(auto& slot:slots_){
            if(slot.list)slot.list->Release();slot.list=nullptr;
            if(slot.allocator)slot.allocator->Release();slot.allocator=nullptr;
        }
        if(event_)CloseHandle(event_);event_=nullptr;
        if(fence_)fence_->Release();fence_=nullptr;
        if(queue_)queue_->Release();queue_=nullptr;
        if(device_)device_->Release();device_=nullptr;
        serial_=0;next_=0;
    }
    HRESULT Initialize(ID3D12CommandQueue* directQueue) noexcept {
        if(queue_==directQueue&&fence_)return S_OK;
        HRESULT hr=Drain();if(FAILED(hr))return hr;
        Release();
        if(!directQueue||directQueue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return E_FAIL;
        queue_=directQueue;queue_->AddRef();
        hr=queue_->GetDevice(IID_PPV_ARGS(&device_));
        for(auto& slot:slots_){
            if(SUCCEEDED(hr))hr=device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&slot.allocator));
            if(SUCCEEDED(hr))hr=device_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,slot.allocator,nullptr,IID_PPV_ARGS(&slot.list));
            if(SUCCEEDED(hr))hr=slot.list->Close();
        }
        if(SUCCEEDED(hr))hr=device_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_));
        if(SUCCEEDED(hr)){event_=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event_)hr=HRESULT_FROM_WIN32(GetLastError());}
        if(FAILED(hr))Release();return hr;
    }
    HRESULT Copy(ID3D12Resource* source,ID3D12Resource* destination) noexcept {
        const auto a=source->GetDesc(),b=destination->GetDesc();
        if(a.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||a.Dimension!=b.Dimension||a.Width!=b.Width||a.Height!=b.Height||a.Format!=b.Format||a.DepthOrArraySize!=b.DepthOrArraySize||a.MipLevels!=b.MipLevels||a.SampleDesc.Count!=b.SampleDesc.Count||a.SampleDesc.Quality!=b.SampleDesc.Quality)return E_INVALIDARG;
        HRESULT hr=Initialize(GetHdr10BridgeDirectQueue());if(FAILED(hr))return hr;
        auto& slot=slots_[next_];
        hr=Retire(slot,true,false);if(FAILED(hr))return hr;
        hr=slot.allocator->Reset();if(SUCCEEDED(hr))hr=slot.list->Reset(slot.allocator,nullptr);
        if(FAILED(hr))return hr;
        D3D12_RESOURCE_BARRIER barriers[2]{};
        for(auto& barrier:barriers){barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COMMON;}
        barriers[0].Transition.pResource=source;barriers[0].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers[1].Transition.pResource=destination;barriers[1].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
        slot.list->ResourceBarrier(2,barriers);slot.list->CopyResource(destination,source);
        for(auto& barrier:barriers){barrier.Transition.StateBefore=barrier.Transition.StateAfter;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COMMON;}
        slot.list->ResourceBarrier(2,barriers);
        hr=slot.list->Close();if(FAILED(hr))return hr;
        source->AddRef();destination->AddRef();slot.source=source;slot.destination=destination;
        slot.value=UINT64_MAX; // Be conservative until queue Signal succeeds.
        ID3D12CommandList* lists[]={slot.list};queue_->ExecuteCommandLists(1,lists);
        const UINT64 value=++serial_;hr=queue_->Signal(fence_,value);
        if(SUCCEEDED(hr)){slot.value=value;++submitted_;next_=(next_+1)%3;}
        return hr;
    }
public:
    FGSDRCorrection()=default;
    FGSDRCorrection(const FGSDRCorrection&)=delete;
    FGSDRCorrection& operator=(const FGSDRCorrection&)=delete;
    ~FGSDRCorrection(){Shutdown();}
    void Shutdown() noexcept {if(shutdown_)return;shutdown_=true;if(SUCCEEDED(Drain()))Release();}
    HRESULT PrepareResize(unsigned long long frame,const char* entry) noexcept {
        const HRESULT hr=Drain();
        Log("FG_SDR_DRAIN frame=%llu entry=%s hr=0x%08lX submitted=%llu retired=%llu reuse_waits=%llu drain_waits=%llu",
            frame,entry,static_cast<unsigned long>(hr),submitted_,retired_,reuseWaits_,drainWaits_);
        if(SUCCEEDED(hr))Release(); // Discard recorded commands before ResizeBuffers.
        return hr;
    }
    HRESULT Apply(const void* wrapper,IDXGISwapChain3* chain,unsigned long long frame) noexcept {
        if(failed_)return E_FAIL;
        const DWORD savedError=GetLastError();copyWait_=false;
        for(auto& slot:slots_){const HRESULT hr=Retire(slot,false,false);if(FAILED(hr)){failed_=true;SetLastError(savedError);return hr;}}
        DXGI_SWAP_CHAIN_DESC1 desc{};ID3D12Resource* buffers[2]{};
        HRESULT hr=chain->GetDesc1(&desc);
        const UINT index=chain->GetCurrentBackBufferIndex();
        if(SUCCEEDED(hr)&&desc.BufferCount==2){
            hr=chain->GetBuffer(0,IID_PPV_ARGS(&buffers[0]));
            if(SUCCEEDED(hr))hr=chain->GetBuffer(1,IID_PPV_ARGS(&buffers[1]));
        }
        const auto native=FGNativeSourceSelect(wrapper,buffers,desc.BufferCount,index);
        const bool verified=SUCCEEDED(hr)&&index<2&&native.status==control_fg_native_source::Status::Matched;
        const bool mismatch=verified&&native.source!=index;
        HRESULT copyHr=S_OK;
        if(mismatch){copyHr=Copy(buffers[native.source],buffers[index]);if(FAILED(copyHr))failed_=true;}
        if(FGAlignActive(frame)||FGPixelNeedsPresent(frame)||(frame%120)==0||FAILED(copyHr))
            Log("FG_SDR_CORRECTION frame=%llu native_index=%u source=%u destination=%u native_resource=%p verified=%u mismatch=%u corrected=%u reason=%s query_hr=0x%08lX copy_hr=0x%08lX copy_wait=%u async=1 submitted=%llu retired=%llu reuse_waits=%llu drain_waits=%llu",
                frame,native.nativeIndex,native.source,index,reinterpret_cast<void*>(native.resource),unsigned(verified),unsigned(mismatch),unsigned(mismatch&&SUCCEEDED(copyHr)),FAILED(hr)?"buffer_query_failed":control_fg_native_source::Name(native.status),static_cast<unsigned long>(hr),static_cast<unsigned long>(copyHr),unsigned(copyWait_),submitted_,retired_,reuseWaits_,drainWaits_);
        for(auto* buffer:buffers)if(buffer)buffer->Release();
        SetLastError(savedError);return copyHr;
    }
};
