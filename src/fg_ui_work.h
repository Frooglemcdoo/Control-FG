#pragma once

// Private direct list: only submitted after the engine flushes its frame.
// A failed/uncertain submission is quarantined; never reset its allocator.
struct FGUIWork {
    ID3D12CommandAllocator* allocator=nullptr;
    ID3D12GraphicsCommandList* list=nullptr;
    ID3D12Fence* fence=nullptr;
    ID3D12CommandQueue* submittedQueue=nullptr;
    HANDLE eventHandle=nullptr;
    UINT64 serial=0, target=0;
    bool pending=false, recording=false, failed=false;

    HRESULT Begin(ID3D12Device* device,UINT64 frame) noexcept {
        if(failed||pending||recording)return E_FAIL;
        if(fence&&serial){
            UINT64 done=fence->GetCompletedValue();
            if(done==UINT64_MAX){failed=true;return DXGI_ERROR_DEVICE_REMOVED;}
            if(done<serial){
                HRESULT hr=fence->SetEventOnCompletion(serial,eventHandle);
                if(FAILED(hr)){failed=true;return hr;}
                if(WaitForSingleObject(eventHandle,5000)!=WAIT_OBJECT_0){failed=true;return E_FAIL;}
                done=fence->GetCompletedValue();
                if(done==UINT64_MAX||done<serial){failed=true;return E_FAIL;}
            }
        }
        HRESULT hr=S_OK;
        if(!allocator){
            hr=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));
            if(SUCCEEDED(hr))hr=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list));
            if(SUCCEEDED(hr))hr=list->Close();
            if(SUCCEEDED(hr))hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
            if(SUCCEEDED(hr)){eventHandle=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!eventHandle)hr=E_FAIL;}
        }
        if(SUCCEEDED(hr))hr=allocator->Reset();
        if(SUCCEEDED(hr))hr=list->Reset(allocator,nullptr);
        if(FAILED(hr)){failed=true;return hr;}
        target=frame;recording=true;return S_OK;
    }
    HRESULT Finish() noexcept {
        if(failed||!recording)return E_FAIL;
        const HRESULT hr=list->Close();
        if(FAILED(hr)){failed=true;return hr;}
        pending=true;recording=false;return S_OK;
    }
    HRESULT Submit(ID3D12CommandQueue* directQueue,UINT64 frame) noexcept {
        if(failed)return E_FAIL;
        if(!pending)return S_OK;
        if(!directQueue||frame!=target){failed=true;return E_UNEXPECTED;}
        if(submittedQueue!=directQueue){
            // Begin already retired the previous use of this context.
            if(submittedQueue)submittedQueue->Release();
            submittedQueue=directQueue;submittedQueue->AddRef();
        }
        ID3D12CommandList* lists[]{list};directQueue->ExecuteCommandLists(1,lists);
        const HRESULT hr=directQueue->Signal(fence,++serial);
        if(FAILED(hr)){failed=true;return hr;}
        pending=false;return S_OK;
    }
};
