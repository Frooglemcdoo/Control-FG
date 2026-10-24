#pragma once
// Included inside native_lighting after the table journal and GPU helpers.
// Native bindings are sampled only inside the verified spotlight pass.
namespace rt_spot_capture_runtime {
using Microsoft::WRL::ComPtr;
namespace gpu=rt_spot_capture_gpu;
enum class Phase : unsigned { Idle,Armed,Recording,Recorded,Submitting,Submitted,Complete,Failed };
inline std::atomic<Phase> phase{Phase::Idle};
inline std::atomic<bool> ready{false};
inline gpu::Resources resources;
inline SRWLOCK stateLock=SRWLOCK_INIT,ownerLock=SRWLOCK_INIT;
inline ComPtr<ID3D12Resource> tableOwner,recordedTable;
inline ComPtr<ID3D12DescriptorHeap> recordedHeap;
inline UINT64 tableBase=0,tableExtent=0;
inline std::atomic<UINT64> wantedAddress{0},wantedBytes{0};
inline UINT64 recordedGeneration=0,recordedPresent=0;
inline ComPtr<ID3D12GraphicsCommandList4> recordedList;
inline ComPtr<ID3D12StateObject> recordedState,captureState;
inline ComPtr<ID3D12CommandQueue> submittedQueue;
inline unsigned attempts=0,exportCursor=0,exportStage=0;
inline UINT64 fenceValue=1;
inline UINT completionStatus[4]{},completionRays=0,completionHits=0;
inline UINT frameTag=0,capturedWidth=0,capturedHeight=0,capturedTableRecords=0;
inline bool readbackCopied=false;
inline unsigned char completedBytes[gpu::kReadbackBytes]{};
inline thread_local bool inScope=false,internal=false;
struct Context {
    void* manager=nullptr; ID3D12GraphicsCommandList4* list=nullptr;
    ID3D12DescriptorHeap* heap=nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{};D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
};
inline thread_local Context mirrored;
inline thread_local UINT64 observed=0;
using Copy=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
using Ranges=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
using Cbv=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using Srv=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using Uav=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
inline Copy originalCopy=nullptr;inline Ranges originalRanges=nullptr;
inline Cbv originalCbv=nullptr;inline Srv originalSrv=nullptr;inline Uav originalUav=nullptr;
inline bool ReadContext(Context& out) noexcept {
    out={};if(!verifiedD3d || !resources.device)return false;
    __try {
        auto* module=reinterpret_cast<unsigned char*>(verifiedD3d);
        const DWORD index=*reinterpret_cast<DWORD*>(module+0x1115fc);
        auto** vector=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        auto* tls=vector[index];if(!tls)return false;
        auto* manager=*reinterpret_cast<unsigned char**>(tls+0x20);
        if(!manager)manager=*reinterpret_cast<unsigned char**>(module+0x111c10);
        auto* context=*reinterpret_cast<unsigned char**>(tls+8);
        if(!context)context=*reinterpret_cast<unsigned char**>(module+0x111c18);
        if(!manager || !context)return false;
        auto* provider=*reinterpret_cast<unsigned char**>(manager+0x50);if(!provider)return false;
        out.list=*reinterpret_cast<ID3D12GraphicsCommandList4**>(context+0x20);
        out.heap=*reinterpret_cast<ID3D12DescriptorHeap**>(provider);
        out.gpu=*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(manager+0x30);
        if(!out.list || !out.heap)return false;
        const auto base=out.heap->GetGPUDescriptorHandleForHeapStart().ptr;
        const auto desc=out.heap->GetDesc();
        if(desc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || !(desc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ||
            out.gpu.ptr<base || (out.gpu.ptr-base)%resources.step)return false;
        const auto slot=(out.gpu.ptr-base)/resources.step;
        if(slot>desc.NumDescriptors || 49>desc.NumDescriptors-slot)return false;
        out.cpu=out.heap->GetCPUDescriptorHandleForHeapStart();out.cpu.ptr+=SIZE_T(slot)*resources.step;
        out.manager=manager;return out.list->GetType()==D3D12_COMMAND_LIST_TYPE_DIRECT;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
inline bool Fresh(const Context& context,D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle,D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle) noexcept {
    __try {
        unsigned char* provider=nullptr;
        using Allocate=void(*)(void*,UINT64,D3D12_GPU_DESCRIPTOR_HANDLE*,D3D12_CPU_DESCRIPTOR_HANDLE*,unsigned char**);
        auto allocate=reinterpret_cast<Allocate>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x1aa00);
        allocate(context.manager,49,&gpuHandle,&cpuHandle,&provider);
        if(!provider || *reinterpret_cast<ID3D12DescriptorHeap**>(provider)!=context.heap)return false;
        const auto base=context.heap->GetGPUDescriptorHandleForHeapStart().ptr;
        if(gpuHandle.ptr<base || (gpuHandle.ptr-base)%resources.step)return false;
        const auto index=(gpuHandle.ptr-base)/resources.step;const auto desc=context.heap->GetDesc();
        if(index<0x3b920 || index>desc.NumDescriptors || 49>desc.NumDescriptors-index)return false;
        auto cpu=context.heap->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(index)*resources.step;
        if(cpu.ptr!=cpuHandle.ptr)return false;
        const auto nativeIndex=(context.gpu.ptr-base)/resources.step;
        return index+49<=nativeIndex || nativeIndex+49<=index;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
inline void Fail(const char* reason) noexcept {
    phase.store(Phase::Failed,std::memory_order_release);
    Trace("rt_hit_gpu_failed reason=%s resources_retained=1",reason);
}
// UI thread only. A failed or still-owned recording is never recycled.
inline bool RequestFresh() noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) return false;
    if(!ready.load())return false;
    AcquireSRWLockExclusive(&stateLock);
    bool accepted=false;
    if(phase.load()==Phase::Idle)accepted=true;
    else if(phase.load()==Phase::Complete && exportStage==3 && readbackCopied) {
        UINT64 generation=0;UINT size=sizeof(generation);
        const bool reset=recordedList && SUCCEEDED(recordedList->GetPrivateData(rt_spot_table_audit::kGeneration,&size,&generation)) && generation!=recordedGeneration;
        const auto completed=resources.fence->GetCompletedValue();
        if(reset && completed!=UINT64_MAX && completed>=fenceValue) {
            ++fenceValue;attempts=exportCursor=exportStage=0;readbackCopied=false;
            recordedList.Reset();recordedState.Reset();captureState.Reset();recordedTable.Reset();recordedHeap.Reset();submittedQueue.Reset();
            accepted=true;
        }
    }
    if(accepted){attempts=0;phase.store(Phase::Armed,std::memory_order_release);}
    ReleaseSRWLockExclusive(&stateLock);
    if(accepted){rt_spot_table_audit::Request();Trace("rt_hit_gpu_requested grid=64 repeatable=1 fence=%llu",fenceValue);}
    return accepted;
}
inline void CancelPending() noexcept {
    Phase expected=Phase::Armed;
    if(phase.compare_exchange_strong(expected,Phase::Idle))Trace("rt_hit_gpu_cancelled reason=suite_ended_before_recording");
}
inline bool Exported() noexcept {return phase.load()==Phase::Complete && exportStage==3;}
inline void BeginScope() noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) return;
    if(inScope || !ready.load())return;
    Phase expected=Phase::Armed;if(!phase.compare_exchange_strong(expected,Phase::Recording))return;
    inScope=true;mirrored={};observed=0;
}
inline void EndScope() noexcept {
    if(!inScope)return;
    inScope=false;
    Phase expected=Phase::Recording;
    if(phase.compare_exchange_strong(expected,Phase::Armed) && ++attempts>=240)Fail("binding_attempt_budget");
}
inline bool Mirror(D3D12_CPU_DESCRIPTOR_HANDLE to,UINT count,UINT& index,UINT& skip,UINT& copied) noexcept {
    if(!inScope || internal || phase.load()!=Phase::Recording)return false;
    Context current;if(!ReadContext(current))return false;
    if(current.list!=mirrored.list || current.heap!=mirrored.heap || current.gpu.ptr!=mirrored.gpu.ptr) {
        mirrored=current;observed=0;internal=true;
        for(UINT i=0;i<5;++i)originalCbv(resources.device.Get(),nullptr,resources.At(i));
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        for(UINT i=5;i<41;++i)originalSrv(resources.device.Get(),nullptr,&srv,resources.At(i));
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32_FLOAT;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        for(UINT i=41;i<49;++i)originalUav(resources.device.Get(),nullptr,nullptr,&uav,resources.At(i));
        internal=false;
    }
    const auto overlap=rt_lights::DescriptorOverlap(mirrored.cpu.ptr,49,to.ptr,count,resources.step);
    index=overlap.tableIndex;skip=overlap.sourceSkip;copied=overlap.count;return copied!=0;
}
inline void MirrorCopy(ID3D12Device* device,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE to,D3D12_CPU_DESCRIPTOR_HANDLE from) noexcept {
    UINT index=0,skip=0,copied=0;if(!Mirror(to,count,index,skip,copied))return;
    from.ptr+=SIZE_T(skip)*resources.step;
    internal=true;originalCopy(device,copied,resources.At(index),from,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);internal=false;
    observed|=((UINT64(1)<<copied)-1)<<index;
}
inline void STDMETHODCALLTYPE CopyHook(ID3D12Device* device,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE to,D3D12_CPU_DESCRIPTOR_HANDLE from,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    originalCopy(device,count,to,from,type);
    if(type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)MirrorCopy(device,count,to,from);
}
inline void STDMETHODCALLTYPE RangesHook(ID3D12Device* device,UINT nd,const D3D12_CPU_DESCRIPTOR_HANDLE* dst,const UINT* dc,
    UINT ns,const D3D12_CPU_DESCRIPTOR_HANDLE* src,const UINT* sc,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    originalRanges(device,nd,dst,dc,ns,src,sc,type);
    if(!inScope || internal || type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)return;
    UINT si=0,so=0;
    for(UINT di=0;di<nd;++di) {
        UINT offset=0,remaining=dc?dc[di]:1;
        while(remaining && si<ns) {
            const UINT available=sc?sc[si]:1;
            if(so==available){++si;so=0;continue;}
            const UINT count=(std::min)(remaining,available-so);
            auto to=dst[di],from=src[si];to.ptr+=SIZE_T(offset)*resources.step;from.ptr+=SIZE_T(so)*resources.step;
            MirrorCopy(device,count,to,from);offset+=count;remaining-=count;so+=count;
        }
    }
}
inline void STDMETHODCALLTYPE CbvHook(ID3D12Device* device,const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE to) {
    originalCbv(device,desc,to);UINT index=0,skip=0,count=0;
    if(Mirror(to,1,index,skip,count)){originalCbv(device,desc,resources.At(index));observed|=UINT64(1)<<index;}
}
inline void STDMETHODCALLTYPE SrvHook(ID3D12Device* device,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE to) {
    originalSrv(device,resource,desc,to);UINT index=0,skip=0,count=0;
    if(Mirror(to,1,index,skip,count)){originalSrv(device,resource,desc,resources.At(index));observed|=UINT64(1)<<index;}
}
inline void STDMETHODCALLTYPE UavHook(ID3D12Device* device,ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE to) {
    originalUav(device,resource,counter,desc,to);UINT index=0,skip=0,count=0;
    if(Mirror(to,1,index,skip,count)){originalUav(device,resource,counter,desc,resources.At(index));observed|=UINT64(1)<<index;}
}
inline void ObserveTableOwner(ID3D12Resource* resource) noexcept {
    const auto wanted=wantedAddress.load(std::memory_order_acquire);
    const auto current=phase.load();if(!wanted || (current!=Phase::Armed && current!=Phase::Recording) || !resource)return;
    const auto desc=resource->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || desc.Width>64ull*1024*1024 || (desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))return;
    const auto address=resource->GetGPUVirtualAddress();
    if(!rt_spot_capture::ContainsGpuRange(address,desc.Width,wanted,wantedBytes.load()) || !TryAcquireSRWLockExclusive(&ownerLock))return;
    tableOwner=resource;tableBase=address;tableExtent=desc.Width;ReleaseSRWLockExclusive(&ownerLock);
}
template<class T> inline bool Attached(ID3D12Object* owner,REFGUID key,ComPtr<T>& object) noexcept {
    UINT size=sizeof(T*);return SUCCEEDED(owner->GetPrivateData(key,&size,object.ReleaseAndGetAddressOf())) && object;
}
inline void Record(ID3D12GraphicsCommandList4* list,ID3D12StateObject* owner,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) return;
    if(!inScope || phase.load()!=Phase::Recording || !desc)return;
    if(desc->Depth!=1 || !desc->Width || desc->Width>16384 || !desc->Height || desc->Height>16384 ||
        !rt_spot_capture::NativeHitTableLayout(desc->HitGroupTable.StartAddress,desc->HitGroupTable.SizeInBytes,desc->HitGroupTable.StrideInBytes))return;
    wantedBytes.store(desc->HitGroupTable.SizeInBytes);wantedAddress.store(desc->HitGroupTable.StartAddress,std::memory_order_release);
    Context current;constexpr UINT64 required=(1ull<<5)|(1ull<<6)|(1ull<<7)|(1ull<<9)|(1ull<<10)|(1ull<<11)|(1ull<<12);
    if(!ReadContext(current) || current.list!=list || current.list!=mirrored.list || current.heap!=mirrored.heap ||
        current.gpu.ptr!=mirrored.gpu.ptr || (observed&required)!=required) {
        if(attempts<4)Trace("rt_hit_gpu_pending reason=descriptor_snapshot observed=%llx required=%llx",observed,required);
        return;
    }
    ComPtr<ID3D12Resource> source;UINT64 offset=0;
    if(!TryAcquireSRWLockShared(&ownerLock))return;
    if(tableOwner && rt_spot_capture::ContainsGpuRange(tableBase,tableExtent,desc->HitGroupTable.StartAddress,desc->HitGroupTable.SizeInBytes)) {
        source=tableOwner;offset=desc->HitGroupTable.StartAddress-tableBase;
    }
    ReleaseSRWLockShared(&ownerLock);
    if(!source){if(attempts<4)Trace("rt_hit_gpu_pending reason=table_resource");return;}
    ComPtr<ID3D12StateObject> inputs,capture;ComPtr<ID3D12PipelineState> translate,clear;
    if(!Attached(owner,gpu::kInputState,inputs) || !Attached(owner,rt_spot_hit_pipeline::kPipeline,capture) ||
        !Attached(owner,gpu::kTranslate,translate) || !Attached(owner,gpu::kClear,clear)){Fail("pipelines_unavailable");return;}
    unsigned char ids[4][32]{},inputId[32]{};UINT size=sizeof(ids);
    if(FAILED(owner->GetPrivateData(rt_spot_hit_pipeline::kIdentifiers,&size,ids)) || size!=sizeof(ids)){Fail("capture_identifiers");return;}
    size=32;if(FAILED(owner->GetPrivateData(gpu::kInputIdentifier,&size,inputId)) || size!=32){Fail("input_identifier");return;}
    ComPtr<ID3D12StateObjectProperties> properties;if(FAILED(owner->QueryInterface(IID_PPV_ARGS(&properties))))return;
    gpu::TableControls tableControls;tableControls.bytes=static_cast<UINT>(desc->HitGroupTable.SizeInBytes);tableControls.count=tableControls.bytes/96;
    for(UINT i=0;i<2;++i) {
        const auto* id=properties->GetShaderIdentifier(i?L"HitGroup_1_0":L"HitGroup_0_0");
        if(!id)return;memcpy(tableControls.nativeIds[i],id,32);memcpy(tableControls.captureIds[i],ids[i],32);
    }
    frameTag=static_cast<UINT>(rt_spot_table_audit::Token(list));if(!frameTag){Fail("frame_tag");return;}
    capturedTableRecords=tableControls.count;
    const UINT inputControls[]={desc->Width,desc->Height,frameTag,64};
    const UINT captureControls[]={gpu::kRayCount,frameTag,0,2,0,1,0,0};
    void* mapped=nullptr;D3D12_RANGE empty{0,0};
    if(FAILED(resources.upload->Map(0,&empty,&mapped))){Fail("upload_map");return;}
    memset(mapped,0,1024);auto* data=static_cast<unsigned char*>(mapped);
    memcpy(data,inputControls,sizeof(inputControls));memcpy(data+256,&tableControls,sizeof(tableControls));memcpy(data+512,captureControls,sizeof(captureControls));
    memcpy(data+768,inputId,32);memcpy(data+832,ids[2],32);memcpy(data+896,ids[3],32);
    D3D12_RANGE written{0,1024};resources.upload->Unmap(0,&written);
    D3D12_GPU_DESCRIPTOR_HANDLE tables[3]{};D3D12_CPU_DESCRIPTOR_HANDLE destinations[3]{};
    for(UINT i=0;i<3;++i)if(!Fresh(current,tables[i],destinations[i])){Fail("private_table_allocation");return;}
    for(UINT i=0;i<3;++i)for(UINT j=0;j<i;++j)
        if(tables[i].ptr<tables[j].ptr+49ull*resources.step && tables[j].ptr<tables[i].ptr+49ull*resources.step){Fail("overlapping_private_tables");return;}
    internal=true;
    for(UINT i=0;i<3;++i) {
        const UINT base=(i+1)*49;
        originalCopy(resources.device.Get(),49,resources.At(base),resources.At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cb{resources.upload->GetGPUVirtualAddress()+i*256,256};originalCbv(resources.device.Get(),&cb,resources.At(base));
    }
    resources.RawUav(resources.inputs.Get(),gpu::kInputAllocationBytes,49+43);
    resources.RawSrv(source.Get(),offset,tableControls.bytes,98+13);
    resources.RawUav(resources.table.Get(),gpu::kTableBytes,98+43);
    resources.RawUav(resources.status.Get(),16,98+44);
    D3D12_SHADER_RESOURCE_VIEW_DESC rays{};rays.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;rays.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    rays.Buffer.NumElements=gpu::kRayCount;rays.Buffer.StructureByteStride=48;
    originalSrv(resources.device.Get(),resources.inputs.Get(),&rays,resources.At(147+13));
    resources.RawSrv(resources.status.Get(),0,16,147+14);
    D3D12_UNORDERED_ACCESS_VIEW_DESC output{};output.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;output.Buffer.NumElements=gpu::kRayCount;output.Buffer.StructureByteStride=80;
    originalUav(resources.device.Get(),resources.results.Get(),nullptr,&output,resources.At(147+43));
    for(UINT i=0;i<3;++i)originalCopy(resources.device.Get(),49,destinations[i],resources.At((i+1)*49),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    internal=false;
    recordedList=list;recordedState=owner;captureState=capture;recordedTable=source;recordedHeap=current.heap;
    recordedGeneration=rt_spot_table_audit::Token(list);recordedPresent=presentCount.load();capturedWidth=desc->Width;capturedHeight=desc->Height;
    const auto hooks=rt_shadow_capture::Lookup(list);
    // Every binding change is local to this command list. Native heap, root,
    // sampler/material tables and root constants remain bound throughout.
    list->SetComputeRootDescriptorTable(0,tables[0]);hooks.set(list,inputs.Get());
    D3D12_DISPATCH_RAYS_DESC inputDispatch{};inputDispatch.RayGenerationShaderRecord={resources.upload->GetGPUVirtualAddress()+768,32};
    inputDispatch.Width=inputDispatch.Height=64;inputDispatch.Depth=1;hooks.dispatch(list,&inputDispatch);
    gpu::Transition(list,resources.inputs.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->SetComputeRootDescriptorTable(0,tables[1]);list->SetPipelineState(clear.Get());list->Dispatch(1,1,1);gpu::UavBarrier(list);
    list->SetPipelineState(translate.Get());list->Dispatch((tableControls.count+63)/64,1,1);
    gpu::Transition(list,resources.table.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    gpu::Transition(list,resources.status.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->SetComputeRootDescriptorTable(0,tables[2]);hooks.set(list,capture.Get());
    D3D12_DISPATCH_RAYS_DESC hitDispatch{};hitDispatch.RayGenerationShaderRecord={resources.upload->GetGPUVirtualAddress()+832,32};
    hitDispatch.MissShaderTable={resources.upload->GetGPUVirtualAddress()+896,32,32};
    hitDispatch.HitGroupTable={resources.table->GetGPUVirtualAddress(),tableControls.bytes,96};
    hitDispatch.Width=gpu::kRayCount;hitDispatch.Height=hitDispatch.Depth=1;hooks.dispatch(list,&hitDispatch);
    gpu::Transition(list,resources.results.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    gpu::Transition(list,resources.status.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(resources.readback.Get(),0,resources.results.Get(),0,gpu::kResultBytes);
    list->CopyBufferRegion(resources.readback.Get(),gpu::kResultBytes,resources.status.Get(),0,16);
    gpu::Transition(list,resources.results.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    gpu::Transition(list,resources.status.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    gpu::Transition(list,resources.inputs.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(resources.readback.Get(),gpu::kLightContextReadbackOffset,resources.inputs.Get(),gpu::kInputBytes,gpu::kLightContextBytes);
    gpu::Transition(list,resources.inputs.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    gpu::Transition(list,resources.table.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    list->SetComputeRootDescriptorTable(0,current.gpu);hooks.set(list,owner);
    phase.store(Phase::Recorded,std::memory_order_release);
    Trace("rt_hit_gpu_recording generation=%llu frame_tag=%u extent=%ux%u rays=4096 table_records=%u native_outputs_untouched=1 bindings_restored=1",recordedGeneration,frameTag,capturedWidth,capturedHeight,tableControls.count);
}
inline void BeforeExecute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    const auto current=phase.load(std::memory_order_acquire);
    if(current<Phase::Recorded || current==Phase::Complete || current==Phase::Failed)return;
    for(UINT i=0;i<count;++i) {
        if(!lists[i])continue;
        UINT64 generation=0;UINT size=sizeof(generation);
        if(FAILED(lists[i]->GetPrivateData(rt_spot_table_audit::kGeneration,&size,&generation)) || generation!=recordedGeneration)continue;
        AcquireSRWLockExclusive(&stateLock);
        if(phase.load()==Phase::Recorded){submittedQueue=queue;phase.store(Phase::Submitting);}
        else if(!readbackCopied)Fail("recording_replayed_before_readback");
        ReleaseSRWLockExclusive(&stateLock);
    }
}
inline void AfterExecute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    if(phase.load()!=Phase::Submitting || queue!=submittedQueue.Get())return;
    bool contains=false;for(UINT i=0;i<count;++i) {
        UINT64 generation=0;UINT size=sizeof(generation);
        contains|=lists[i] && SUCCEEDED(lists[i]->GetPrivateData(rt_spot_table_audit::kGeneration,&size,&generation)) && generation==recordedGeneration;
    }
    if(!contains)return;
    const HRESULT hr=queue->Signal(resources.fence.Get(),fenceValue);
    AcquireSRWLockExclusive(&stateLock);
    if(phase.load()==Phase::Submitting) {
        if(SUCCEEDED(hr)){phase.store(Phase::Submitted);Trace("rt_hit_gpu_submitted generation=%llu fence=%llu",recordedGeneration,fenceValue);}
        else Fail("completion_signal");
    }
    ReleaseSRWLockExclusive(&stateLock);
}
inline void ResetObserved(ID3D12GraphicsCommandList* list) noexcept {
    if(phase.load(std::memory_order_acquire)!=Phase::Recorded)return;
    UINT64 generation=0;UINT size=sizeof(generation);
    if(SUCCEEDED(list->GetPrivateData(rt_spot_table_audit::kGeneration,&size,&generation)) && generation==recordedGeneration)Fail("recording_discarded_before_submit");
}
inline void Poll() noexcept {
    if(phase.load()==Phase::Submitted && TryAcquireSRWLockExclusive(&stateLock)) {
        const auto completed=resources.fence->GetCompletedValue();
        if(completed==UINT64_MAX)Fail("device_removed");
        else if(completed>=fenceValue && phase.load()==Phase::Submitted) {
            void* mapped=nullptr;D3D12_RANGE range{0,gpu::kReadbackBytes};
            if(FAILED(resources.readback->Map(0,&range,&mapped)))Fail("readback_map");
            else {
                memcpy(completedBytes,mapped,sizeof(completedBytes));D3D12_RANGE written{0,0};resources.readback->Unmap(0,&written);
                readbackCopied=true;phase.store(Phase::Complete);
                UINT status[4]{};memcpy(status,completedBytes+gpu::kResultBytes,16);
                const bool tableValid=!status[0] && !status[3] && status[1]+status[2]==capturedTableRecords;
                UINT validRays=0,hits=0,invalidRecords=0;
                for(UINT i=0;i<gpu::kRayCount;++i) {
                    rt_spot_capture::CaptureResult result{};memcpy(&result,completedBytes+i*sizeof(result),sizeof(result));
                    if(!result.valid)continue;
                    if(result.valid!=1 || result.frameTag!=frameTag || result.pixelX>=capturedWidth || result.pixelY>=capturedHeight ||
                        (result.hit.hitKind!=0 && result.hit.hitKind!=254 && result.hit.hitKind!=255)){++invalidRecords;continue;}
                    ++validRays;if(result.hit.hitKind)++hits;
                }
                Trace("rt_hit_gpu_readback generation=%llu frame_tag=%u unknown=%u translated=%u null=%u invalid_layout=%u valid_rays=%u hits=%u misses=%u invalid_records=%u gpu_completion_confirmed=1 gpu_hit_capture=%u",recordedGeneration,frameTag,status[0],status[1],status[2],status[3],validRays,hits,validRays-hits,invalidRecords,unsigned(tableValid && !invalidRecords));
                memcpy(completionStatus,status,sizeof(status));completionRays=validRays;completionHits=hits;
                if(!tableValid)Fail("gpu_table_validation");
                else if(invalidRecords)Fail("readback_provenance");
            }
        }
        ReleaseSRWLockExclusive(&stateLock);
    }
    if(phase.load()!=Phase::Complete || exportStage==3)return;
    // Retry canonical headers and records if the nonblocking log queue is busy.
    // Cursor advancement means successful enqueue, never merely an attempted write.
    if(exportStage==0) {
        if(!Trace("rt_hit_gpu_recorded generation=%llu frame_tag=%u extent=%ux%u table_records=%u captured_present=%llu light_context_bytes=128",recordedGeneration,frameTag,capturedWidth,capturedHeight,capturedTableRecords,recordedPresent))return;
        exportStage=1;
    }
    if(exportStage==1) {
        if(!Trace("rt_hit_gpu_complete generation=%llu frame_tag=%u unknown=%u translated=%u null=%u invalid_layout=%u valid_rays=%u hits=%u misses=%u invalid_records=0 gpu_completion_confirmed=1 gpu_hit_capture=1",recordedGeneration,frameTag,completionStatus[0],completionStatus[1],completionStatus[2],completionStatus[3],completionRays,completionHits,completionRays-completionHits))return;
        exportStage=2;
    }
    // Bounded batches share the existing asynchronous log worker, never the render thread.
    const unsigned end=(std::min)(exportCursor+64,gpu::kRayCount);
    for(;exportCursor<end;++exportCursor) {
        const auto* record=completedBytes+exportCursor*80;UINT valid=0;memcpy(&valid,record+76,4);
        if(valid) {
            char hex[161]{};rt_spot_capture_audit::Hex(record,80,hex);
            char contextHex[gpu::kLightContextStride*2+1]{};
            rt_spot_capture_audit::Hex(completedBytes+gpu::kLightContextReadbackOffset+exportCursor*gpu::kLightContextStride,gpu::kLightContextStride,contextHex);
            if(!Trace("rt_hit_gpu_record index=%u bytes=%s light_context=%s",exportCursor,hex,contextHex))return;
        }
    }
    if(exportCursor==gpu::kRayCount && Trace("rt_hit_gpu_export_complete records_examined=4096 format=CaptureResult80 frame_tag=%u",frameTag))exportStage=3;
}
inline void Install(ID3D12Device* device) noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) {
        static std::atomic<bool> reported{false};
        if (!reported.exchange(true))
            Trace("rt_hit_gpu_setup ready=0 disabled=build_policy descriptor_hooks=0 gpu_allocations=0");
        return;
    }
    static bool attempted=false;if(attempted)return;attempted=true;
    const HRESULT hr=resources.Initialize(device);
    if(FAILED(hr)){Trace("rt_hit_gpu_setup ready=0 hr=0x%08lX",static_cast<unsigned long>(hr));return;}
    auto** table=*reinterpret_cast<void***>(device);
    originalCopy=reinterpret_cast<Copy>(table[24]);originalRanges=reinterpret_cast<Ranges>(table[23]);
    originalCbv=reinterpret_cast<Cbv>(table[17]);originalSrv=reinterpret_cast<Srv>(table[18]);originalUav=reinterpret_cast<Uav>(table[19]);
    bool installed=PatchSlot(&table[24],reinterpret_cast<void*>(originalCopy),reinterpret_cast<void*>(&CopyHook));
    installed=PatchSlot(&table[23],reinterpret_cast<void*>(originalRanges),reinterpret_cast<void*>(&RangesHook))&&installed;
    installed=PatchSlot(&table[17],reinterpret_cast<void*>(originalCbv),reinterpret_cast<void*>(&CbvHook))&&installed;
    installed=PatchSlot(&table[18],reinterpret_cast<void*>(originalSrv),reinterpret_cast<void*>(&SrvHook))&&installed;
    installed=PatchSlot(&table[19],reinterpret_cast<void*>(originalUav),reinterpret_cast<void*>(&UavHook))&&installed;
    ready.store(installed);Trace("rt_hit_gpu_setup ready=%u bounded_table_bytes=%u",unsigned(installed),gpu::kTableBytes);
}
}
