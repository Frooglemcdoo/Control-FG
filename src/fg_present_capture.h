#pragma once
// F9-only SDR presentation readback. The deliberate fence wait avoids retaining
// a swapchain buffer across a later resize; this test can affect race timing.
static void FGPixelSDRPresent(IDXGISwapChain3* chain,unsigned long long frame) noexcept {
    static bool failed=false;
    if(failed||!chain||!FGPixelNeedsPresent(frame))return;
    auto queue=GetHdr10BridgeDirectQueue();if(!queue)return;
    ID3D12Device* device=nullptr;ID3D12Resource* buffer=nullptr;
    ID3D12CommandAllocator* allocator=nullptr;ID3D12GraphicsCommandList* list=nullptr;ID3D12Fence* fence=nullptr;
    HANDLE event=nullptr;bool submitted=false,completed=false;
    const UINT index=chain->GetCurrentBackBufferIndex();
    HRESULT hr=chain->GetBuffer(index,IID_PPV_ARGS(&buffer));
    if(SUCCEEDED(hr))hr=queue->GetDevice(IID_PPV_ARGS(&device));
    if(SUCCEEDED(hr))hr=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));
    if(SUCCEEDED(hr))hr=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list));
    if(SUCCEEDED(hr))hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
    if(SUCCEEDED(hr)){event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)hr=HRESULT_FROM_WIN32(GetLastError());}
    if(SUCCEEDED(hr)){
        FGPixelBridge(list,buffer,frame,index);
        // F9 captures both current SDR buffers on the same queue and fence as
        // the selected presentation buffer. No reference survives completion.
        DXGI_SWAP_CHAIN_DESC1 desc{};
        if(SUCCEEDED(chain->GetDesc1(&desc))&&desc.BufferCount==2){
            for(UINT candidate=0;candidate<2;++candidate){
                ID3D12Resource* sample=nullptr;
                const HRESULT sampleHr=chain->GetBuffer(candidate,IID_PPV_ARGS(&sample));
                if(SUCCEEDED(sampleHr)&&sample){FGPixelBridge(list,sample,frame,candidate,6+candidate);sample->Release();}
                else Log("PIXEL_INVALID frame=%llu reason=sdr_buffer_query_failed index=%u hr=0x%08lX",frame,candidate,static_cast<unsigned long>(sampleHr));
            }
        }
        hr=list->Close();
        if(SUCCEEDED(hr)){
            ID3D12CommandList* lists[]={list};queue->ExecuteCommandLists(1,lists);submitted=true;
            hr=queue->Signal(fence,1);
            if(SUCCEEDED(hr))hr=fence->SetEventOnCompletion(1,event);
            if(SUCCEEDED(hr)){
                const DWORD result=WaitForSingleObject(event,5000);
                completed=result==WAIT_OBJECT_0&&fence->GetCompletedValue()!=UINT64_MAX;
                if(!completed)hr=E_FAIL;
            }
        }
    }
    if(completed)FGPixelReleaseCompletedSources(list);
    Log("PIXEL_SDR_PRESENT frame=%llu index=%u source=%p queue=%p hr=0x%08lX completed=%u capture_wait=1",frame,index,buffer,queue,static_cast<unsigned long>(hr),unsigned(completed));
    if(submitted&&!completed){
        // Device failure/timeout: never reuse or release in-flight capture objects.
        failed=true;Log("PIXEL_INVALID frame=%llu reason=sdr_capture_fence_failed action=quarantine_disable_sdr_capture",frame);
        if(device)device->Release();return;
    }
    if(event)CloseHandle(event);if(fence)fence->Release();if(list)list->Release();
    if(allocator)allocator->Release();if(buffer)buffer->Release();if(device)device->Release();
}
