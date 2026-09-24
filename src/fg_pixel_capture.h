#pragma once
// Diagnostic only: nine small color/mask tiles in SDR or HDR, four consecutive frames per F9 burst.
// Fence completion is observed on the queue actually submitting each command list.
static ID3D12CommandQueue* GetHdr10BridgeDirectQueue() noexcept;
static bool IsHdr10BridgeActive() noexcept;
using FGPixelExecute=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
using FGPixelReset=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
struct FGPixelHook {void** table=nullptr;void* original=nullptr;};
struct FGPixelJob {
    unsigned phase=0,burst=0,ordinal=0,role=0; // 1 recorded, 2 submitting, 3 fenced, 4 writing, 5 done, 6 quarantined
    unsigned long long frame=0,epoch=0;
    UINT width=0,height=0,index=UINT_MAX,format=0,bpp=0,hdr=0;
    const void* sourceIdentity=nullptr;ID3D12Resource* source=nullptr;ID3D12Resource* readback=nullptr;
    ID3D12CommandList* list=nullptr;ID3D12Fence* fence=nullptr;ID3D12CommandQueue* queue=nullptr;
};
static SRWLOCK fgPixelLock=SRWLOCK_INIT;
static FGPixelHook fgPixelQueues[8]{},fgPixelResets[8]{};
static FGPixelJob fgPixelJobs[160]{}; // Four bursts, at most 90 MiB of readback storage.
static unsigned fgPixelBurst=0,fgPixelRemaining=0,fgPixelOrdinal=0;
static unsigned long long fgPixelEpoch=0,fgPixelLastFrame=0,fgPixelDeadline=0;
static constexpr UINT fgPixelW=128,fgPixelH=64,fgPixelPitch=1024;
static constexpr UINT64 fgPixelBytes=UINT64(9)*fgPixelH*fgPixelPitch;
static UINT FGPixelBpp(DXGI_FORMAT f) noexcept {
    switch(f){case DXGI_FORMAT_R16G16B16A16_FLOAT:return 8;
    case DXGI_FORMAT_R10G10B10A2_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return 4;
    case DXGI_FORMAT_R8_UNORM:return 1;default:return 0;}
}
static const char* FGPixelRole(unsigned role) noexcept {
    const char* roles[]={"producer","bridge","hudless","ui_alpha","tagged_pq","final_pq","shadow0","shadow1","shadow2","shadow3"};return role<10?roles[role]:"unknown";
}
static bool FGPixelPatch(void** slot,void* value) noexcept {
    DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
    InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(slot),value);
    DWORD unused=0;VirtualProtect(slot,sizeof(void*),old,&unused);return true;
}
static void STDMETHODCALLTYPE FGPixelExecuteHook(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists) {
    auto table=*reinterpret_cast<void***>(queue);FGPixelExecute fn=nullptr;unsigned selected[160]{},n=0;
    AcquireSRWLockExclusive(&fgPixelLock);
    for(const auto& h:fgPixelQueues)if(h.table==table)fn=reinterpret_cast<FGPixelExecute>(h.original);
    for(unsigned i=0;i<160;++i){auto& j=fgPixelJobs[i];if(j.phase!=1)continue;
        for(UINT k=0;k<count;++k)if(j.list==lists[k]){j.phase=2;j.queue=queue;selected[n++]=i;break;}}
    ReleaseSRWLockExclusive(&fgPixelLock);
    if(!fn)return; // Publication installs the original before the hook can run.
    fn(queue,count,lists);
    AcquireSRWLockExclusive(&fgPixelLock);
    for(unsigned k=0;k<n;++k){auto& j=fgPixelJobs[selected[k]];const HRESULT hr=queue->Signal(j.fence,1);
        j.phase=SUCCEEDED(hr)?3:6;
        Log("PIXEL_SUBMIT burst=%u ordinal=%u role=%u frame=%llu queue=%p list=%p hr=0x%08lX",j.burst,j.ordinal,j.role,j.frame,queue,j.list,static_cast<unsigned long>(hr));}
    ReleaseSRWLockExclusive(&fgPixelLock);
}
static HRESULT STDMETHODCALLTYPE FGPixelResetHook(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso) {
    auto table=*reinterpret_cast<void***>(list);FGPixelReset fn=nullptr;
    AcquireSRWLockShared(&fgPixelLock);for(const auto& h:fgPixelResets)if(h.table==table)fn=reinterpret_cast<FGPixelReset>(h.original);ReleaseSRWLockShared(&fgPixelLock);
    if(!fn)return E_UNEXPECTED;const HRESULT hr=fn(list,allocator,pso);
    if(SUCCEEDED(hr)){AcquireSRWLockExclusive(&fgPixelLock);
        for(auto& j:fgPixelJobs)if(j.list==list&&j.phase==1){j.phase=6;Log("PIXEL_INVALID burst=%u ordinal=%u role=%u reason=reset_before_observed_submit",j.burst,j.ordinal,j.role);}
        ReleaseSRWLockExclusive(&fgPixelLock);}
    return hr;
}
static bool FGPixelEnsureHooks(ID3D12CommandQueue* queue,ID3D12GraphicsCommandList* list) noexcept {
    if(!queue||!list)return false;
    auto qt=*reinterpret_cast<void***>(queue);auto lt=*reinterpret_cast<void***>(list);
    bool qok=false,lok=false;
    for(auto& h:fgPixelQueues)if(h.table==qt)qok=true;
    if(!qok)for(auto& h:fgPixelQueues)if(!h.table){h.table=qt;h.original=qt[10];qok=FGPixelPatch(&qt[10],reinterpret_cast<void*>(&FGPixelExecuteHook));if(!qok)h={};break;}
    for(auto& h:fgPixelResets)if(h.table==lt)lok=true;
    if(!lok)for(auto& h:fgPixelResets)if(!h.table){h.table=lt;h.original=lt[10];lok=FGPixelPatch(&lt[10],reinterpret_cast<void*>(&FGPixelResetHook));if(!lok)h={};break;}
    return qok&&lok;
}
static bool FGPixelRecord(ID3D12GraphicsCommandList* list,ID3D12Resource* source,D3D12_RESOURCE_STATES state,
    unsigned long long frame,unsigned burst,unsigned ordinal,unsigned role,UINT index) noexcept {
    // Caller owns fgPixelLock. Allocation is completed before adding any GPU commands.
    FGPixelJob* job=nullptr;for(auto& j:fgPixelJobs)if(j.phase==0){job=&j;break;}if(!job)return false;
    const auto desc=source->GetDesc();
    if(!FGPixelBpp(desc.Format)||desc.Width<fgPixelW||desc.Width>16384||desc.Height<fgPixelH||desc.Height>16384||desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.DepthOrArraySize!=1||desc.MipLevels!=1||desc.SampleDesc.Count!=1)return false;
    if(!FGPixelEnsureHooks(GetHdr10BridgeDirectQueue(),list))return false;
    ID3D12Device* device=nullptr;if(FAILED(source->GetDevice(IID_PPV_ARGS(&device))))return false;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=1;heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=fgPixelBytes;buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* readback=nullptr;ID3D12Fence* fence=nullptr;
    HRESULT hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback));
    if(SUCCEEDED(hr))hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));device->Release();
    if(FAILED(hr)){if(readback)readback->Release();if(fence)fence->Release();return false;}
    job->phase=1;job->burst=burst;job->ordinal=ordinal;job->role=role;job->frame=frame;job->epoch=fgAlignEpoch.load();job->width=static_cast<UINT>(desc.Width);job->height=desc.Height;job->index=index;job->format=unsigned(desc.Format);job->bpp=FGPixelBpp(desc.Format);job->hdr=unsigned(IsHdr10BridgeActive());
    job->sourceIdentity=source;job->source=source;source->AddRef();job->readback=readback;job->fence=fence;job->list=list;list->AddRef();
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=source;barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=state;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    if(state!=D3D12_RESOURCE_STATE_COPY_SOURCE)list->ResourceBarrier(1,&barrier);
    for(UINT t=0;t<9;++t){UINT x=(job->width-fgPixelW)*(t%3)/2,y=(job->height-fgPixelH)*(t/3)/2;
        D3D12_BOX box{x,y,0,x+fgPixelW,y+fgPixelH,1};D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=source;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.pResource=readback;dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint.Offset=UINT64(t)*fgPixelH*fgPixelPitch;dst.PlacedFootprint.Footprint.Format=desc.Format;dst.PlacedFootprint.Footprint.Width=fgPixelW;dst.PlacedFootprint.Footprint.Height=fgPixelH;dst.PlacedFootprint.Footprint.Depth=1;dst.PlacedFootprint.Footprint.RowPitch=fgPixelPitch;
        list->CopyTextureRegion(&dst,0,0,0,&src,&box);}
    if(state!=D3D12_RESOURCE_STATE_COPY_SOURCE){std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);}
    Log("PIXEL_RECORD burst=%u ordinal=%u role=%u frame=%llu epoch=%llu source=%p width=%u height=%u format=%u hdr=%u index=%u bytes=%llu",burst,ordinal,role,frame,job->epoch,source,job->width,job->height,job->format,job->hdr,index,fgPixelBytes);
    return true;
}
static void FGPixelProducer(ID3D12GraphicsCommandList* list,ID3D12Resource* source,D3D12_RESOURCE_STATES state,unsigned long long frame,ID3D12Resource* hudless,D3D12_RESOURCE_STATES hudState,ID3D12Resource* alpha,D3D12_RESOURCE_STATES alphaState,ID3D12Resource* pq,D3D12_RESOURCE_STATES pqState) noexcept {
    AcquireSRWLockExclusive(&fgPixelLock);
    if(fgPixelRemaining&&(fgPixelEpoch!=fgAlignEpoch.load()||GetTickCount64()>fgPixelDeadline)){Log("PIXEL_ABORT burst=%u reason=epoch_or_deadline",fgPixelBurst);fgPixelRemaining=0;}
    if(fgPixelRemaining&&frame!=fgPixelLastFrame){
        if(fgPixelOrdinal&&frame!=fgPixelLastFrame+1){Log("PIXEL_ABORT burst=%u reason=nonconsecutive_producer",fgPixelBurst);fgPixelRemaining=0;}
        else if(FGPixelRecord(list,source,state,frame,fgPixelBurst,fgPixelOrdinal+1,0,UINT_MAX)){fgPixelLastFrame=frame;++fgPixelOrdinal;--fgPixelRemaining;
            bool complete=FGPixelRecord(list,hudless,hudState,frame,fgPixelBurst,fgPixelOrdinal,2,UINT_MAX);
            complete=FGPixelRecord(list,alpha,alphaState,frame,fgPixelBurst,fgPixelOrdinal,3,UINT_MAX)&&complete;
            if(pq)complete=FGPixelRecord(list,pq,pqState,frame,fgPixelBurst,fgPixelOrdinal,4,UINT_MAX)&&complete;
            if(!complete){Log("PIXEL_ABORT burst=%u reason=partial_input_capture",fgPixelBurst);fgPixelRemaining=0;}}
        else{Log("PIXEL_ABORT burst=%u reason=producer_setup_failed",fgPixelBurst);fgPixelRemaining=0;}}
    ReleaseSRWLockExclusive(&fgPixelLock);
}
static void FGPixelBridge(ID3D12GraphicsCommandList* list,ID3D12Resource* source,unsigned long long frame,UINT index,unsigned role=1) noexcept {
    AcquireSRWLockExclusive(&fgPixelLock);
    unsigned burst=0,ordinal=0;bool duplicate=false;
    for(const auto& j:fgPixelJobs)if(j.phase&&j.frame==frame&&j.epoch==fgAlignEpoch.load()){
        if(j.role==0&&j.phase!=6){burst=j.burst;ordinal=j.ordinal;}if(j.role==role)duplicate=true;}
    if(burst&&!duplicate&&!FGPixelRecord(list,source,D3D12_RESOURCE_STATE_COMMON,frame,burst,ordinal,role,index))Log("PIXEL_BRIDGE_MISSING burst=%u ordinal=%u frame=%llu",burst,ordinal,frame);
    ReleaseSRWLockExclusive(&fgPixelLock);
}
static void FGPixelPoll() noexcept {
    static bool held=false;const bool down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    AcquireSRWLockExclusive(&fgPixelLock);
    if(down&&!held){
        if(fgPixelRemaining||fgPixelBurst>=4)Log("PIXEL_REQUEST_REJECT reason=busy_or_four_burst_limit");
        else{++fgPixelBurst;fgPixelOrdinal=0;fgPixelRemaining=4;fgPixelEpoch=fgAlignEpoch.load();fgPixelLastFrame=0;fgPixelDeadline=GetTickCount64()+10000;
            FGAlignArm(presentCount.load(),"F9_pixel_capture");Log("PIXEL_REQUEST burst=%u epoch=%llu frames=4 hdr=%u",fgPixelBurst,fgPixelEpoch,unsigned(IsHdr10BridgeActive()));}}
    held=down;
    FGPixelJob* ready=nullptr;
    for(auto& j:fgPixelJobs)if(j.phase==3){const UINT64 done=j.fence->GetCompletedValue();if(done==UINT64_MAX){j.phase=6;Log("PIXEL_INVALID burst=%u ordinal=%u role=%u reason=device_removed",j.burst,j.ordinal,j.role);}else if(done>=1){j.phase=4;ready=&j;break;}}
    ReleaseSRWLockExclusive(&fgPixelLock);
    if(!ready)return;auto& j=*ready;
    wchar_t root[32768]{};DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",root,_countof(root));bool ok=false;
    if(len&&len<_countof(root)){
        std::wstring dir=std::wstring(root)+L"\\ControlFGProbe\\Native-R12-MFG-Dynamic-Test-"+std::to_wstring(GetCurrentProcessId());
        if(CreateDirectoryW(dir.c_str(),nullptr)||GetLastError()==ERROR_ALREADY_EXISTS){
            std::wstring stem=dir+L"\\b"+std::to_wstring(j.burst)+L"-f"+std::to_wstring(j.frame)+L"-"+std::to_wstring(j.role);
            void* data=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(fgPixelBytes)};
            if(SUCCEEDED(j.readback->Map(0,&range,&data))&&data){HANDLE file=CreateFileW((stem+L".raw").c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
                if(file!=INVALID_HANDLE_VALUE){std::vector<unsigned char> packed(static_cast<size_t>(fgPixelBytes),0);
                    for(UINT row=0;row<9*fgPixelH;++row)std::memcpy(packed.data()+size_t(row)*fgPixelPitch,static_cast<const unsigned char*>(data)+size_t(row)*fgPixelPitch,fgPixelW*j.bpp);
                    DWORD written=0;ok=WriteFile(file,packed.data(),static_cast<DWORD>(fgPixelBytes),&written,nullptr)&&written==fgPixelBytes;CloseHandle(file);}D3D12_RANGE none{0,0};j.readback->Unmap(0,&none);}
            if(ok){char meta[1024];int size=_snprintf_s(meta,sizeof(meta),_TRUNCATE,"{\"burst\":%u,\"ordinal\":%u,\"frame\":%llu,\"epoch\":%llu,\"role\":\"%s\",\"width\":%u,\"height\":%u,\"format\":%u,\"hdr\":%u,\"tiles\":9,\"tile_width\":128,\"tile_height\":64,\"row_pitch\":1024,\"index\":%u,\"source\":\"%p\",\"queue\":\"%p\"}\n",j.burst,j.ordinal,j.frame,j.epoch,FGPixelRole(j.role),j.width,j.height,j.format,j.hdr,j.index,j.sourceIdentity,j.queue);
                HANDLE file=CreateFileW((stem+L".json").c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)ok=false;else{DWORD written=0;ok=size>0&&WriteFile(file,meta,size,&written,nullptr)&&written==static_cast<DWORD>(size);CloseHandle(file);}}}}
    Log("PIXEL_SAVED burst=%u ordinal=%u role=%u frame=%llu success=%u gpu_fence_complete=1",j.burst,j.ordinal,j.role,j.frame,unsigned(ok));
    AcquireSRWLockExclusive(&fgPixelLock);j.readback->Release();if(j.source)j.source->Release();j.list->Release();j.fence->Release();j.readback=nullptr;j.source=nullptr;j.list=nullptr;j.fence=nullptr;j.phase=5;ReleaseSRWLockExclusive(&fgPixelLock);
}

// Capture interest is checked before touching the SDR swapchain or allocating work.
static bool FGPixelNeedsPresent(unsigned long long frame) noexcept {
    bool wanted=false;AcquireSRWLockShared(&fgPixelLock);
    for(const auto& j:fgPixelJobs)if(j.phase&&j.phase!=6&&j.role==0&&j.frame==frame&&j.epoch==fgAlignEpoch.load())wanted=true;
    ReleaseSRWLockShared(&fgPixelLock);return wanted;
}
// Called only after the capture queue fence completes. Keep the address for metadata,
// but release the SDR backbuffer reference before returning to the game/ResizeBuffers.
static void FGPixelReleaseCompletedSources(ID3D12CommandList* list) noexcept {
    AcquireSRWLockExclusive(&fgPixelLock);
    for(auto& j:fgPixelJobs)if(j.list==list&&j.source&&j.phase==3){
        const auto done=j.fence->GetCompletedValue();
        if(done>=1&&done!=UINT64_MAX){j.source->Release();j.source=nullptr;}}
    ReleaseSRWLockExclusive(&fgPixelLock);
}
