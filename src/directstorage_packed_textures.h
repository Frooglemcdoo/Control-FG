#pragma once
#include "native_texture_layout.h"
#include <windows.h>
#include <dstorage.h>
#include "../build/native_texture_repack_bytecode.h"
#include <dxgi.h>
#include <wrl/client.h>
namespace control_storage {
// One event-driven batch owner. Payloads are never mapped by the CPU.
// Caller holds exclusive engine ownership of each unpublished destination until
// Poll completes, and supplies its actual before/after state. Destinations must be newly
// allocated/unpublished or otherwise exclusively handed off to this transport.
class PackedTextureBatch {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
public:
    static constexpr unsigned Capacity=16;
    static constexpr UINT64 StagingBudget=128ull*1024*1024;
    struct InitTiming {UINT64 queuesUs=0,pipelineUs=0,commandsUs=0;bool sharedPipeline=false;};
    struct Input {IDStorageFile* file=nullptr;ID3D12Resource* texture=nullptr;UINT64 offset=0;UINT bytes=0;D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON;};
    enum class Phase {Idle,Storage,Gpu,FailedSubmission};
private:
    struct Item {
        Ptr<IDStorageFile> file;Ptr<ID3D12Resource> target,packed,padded;
        native_texture_layout::Layout layout{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[16]{};
        D3D12_RESOURCE_STATES state{};UINT64 paddedBytes=0,packedOffset=0;
    } items[Capacity];
    Ptr<ID3D12Device> device;Ptr<ID3D12CommandQueue> graphics,transferQueue;Ptr<IDStorageQueue2> storage;
    Ptr<IDStorageStatusArray> status;Ptr<ID3D12RootSignature> root;Ptr<ID3D12PipelineState> pipeline;
    Ptr<ID3D12CommandAllocator> allocator;Ptr<ID3D12GraphicsCommandList> list;Ptr<ID3D12Fence> fence;
    HANDLE storageEvent=nullptr,gpuEvent=nullptr;UINT64 fenceValue=0;unsigned count=0;Phase phase=Phase::Idle;
    HRESULT failure=S_OK;InitTiming initTiming{};
    static UINT64 NowUs(){static const UINT64 f=[](){LARGE_INTEGER n;QueryPerformanceFrequency(&n);return UINT64(n.QuadPart);}();LARGE_INTEGER t;QueryPerformanceCounter(&t);return UINT64(t.QuadPart)/f*1000000+(UINT64(t.QuadPart)%f)*1000000/f;}
    Ptr<ID3D12Resource> packedArena,paddedArena;
    UINT64 packedCapacity=0,paddedCapacity=0,bufferAllocations=0,reusedBatches=0;
    static UINT64 Aligned(UINT64 n,UINT64 alignment){return (n+alignment-1)&~(alignment-1);}
    static UINT64 Growth(UINT64 need){UINT64 n=1024*1024;while(n<need)n*=2;return n;}
    HRESULT EnsureArenas(UINT64 packed,UINT64 padded){
        packed=Aligned(packed,65536);padded=Aligned(padded,65536);
        if(packed+padded>StagingBudget)return E_OUTOFMEMORY;
        if(packed<=packedCapacity&&padded<=paddedCapacity){++reusedBatches;return S_OK;}
        UINT64 a=packedCapacity>=packed?packedCapacity:Growth(packed);
        UINT64 b=paddedCapacity>=padded?paddedCapacity:Growth(padded);
        if(a+b>StagingBudget){a=packed;b=padded;}
        // Submit is idle: storage and GPU fences retired before these arenas
        // can be resized. Release old allocations first to bound peak memory.
        if(a!=packedCapacity){packedArena.Reset();packedCapacity=0;}
        if(b!=paddedCapacity){paddedArena.Reset();paddedCapacity=0;}
        HRESULT hr=S_OK;
        if(!packedArena){hr=Buffer(a,D3D12_RESOURCE_FLAG_NONE,&packedArena);if(FAILED(hr))return hr;packedCapacity=a;++bufferAllocations;}
        if(!paddedArena){hr=Buffer(b,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,&paddedArena);if(FAILED(hr))return hr;paddedCapacity=b;++bufferAllocations;}
        return S_OK;
    }
    static bool Same(IUnknown* a,IUnknown* b){Ptr<IUnknown> x,y;return a&&b&&SUCCEEDED(a->QueryInterface(IID_PPV_ARGS(&x)))&&SUCCEEDED(b->QueryInterface(IID_PPV_ARGS(&y)))&&x.Get()==y.Get();}
    HRESULT Buffer(UINT64 size,D3D12_RESOURCE_FLAGS flags,ID3D12Resource** out){
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;
        D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;
        return device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(out));
    }
    void Barrier(ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
        if(before==after)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
    }
    void Clear(){for(auto& i:items)i={};count=0;phase=Phase::Idle;}
    HRESULT Upload(){
        HRESULT hr=allocator->Reset();if(FAILED(hr))return hr;hr=list->Reset(allocator.Get(),pipeline.Get());if(FAILED(hr))return hr;
        list->SetComputeRootSignature(root.Get());
        Barrier(packedArena.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Barrier(paddedArena.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        for(unsigned j=0;j<count;++j){auto& i=items[j];
            list->SetComputeRootShaderResourceView(0,i.packed->GetGPUVirtualAddress());list->SetComputeRootUnorderedAccessView(1,i.padded->GetGPUVirtualAddress());
            for(unsigned m=0;m<i.layout.count;++m){const auto& l=i.layout.mips[m];const auto& fp=i.footprints[m];
                const UINT data[]={UINT(i.packedOffset+l.compactOffset),UINT(fp.Offset),UINT(l.rowBytes/4),l.rows,fp.Footprint.RowPitch/4};
                list->SetComputeRoot32BitConstants(2,5,data,0);list->Dispatch((data[2]+63)/64,data[3],1);
            }
        }
        Barrier(paddedArena.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        for(unsigned j=0;j<count;++j){auto& i=items[j];
            Barrier(i.target.Get(),i.state,D3D12_RESOURCE_STATE_COPY_DEST);
            for(unsigned m=0;m<i.layout.count;++m){D3D12_TEXTURE_COPY_LOCATION dst{},src{};
                dst.pResource=i.target.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=m;
                src.pResource=i.padded.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=i.footprints[m];
                list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            }
            Barrier(i.target.Get(),D3D12_RESOURCE_STATE_COPY_DEST,i.state);
        }
        Barrier(packedArena.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COMMON);
        Barrier(paddedArena.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
        hr=list->Close();if(FAILED(hr))return hr;
        ID3D12CommandList* lists[]={list.Get()};transferQueue->ExecuteCommandLists(1,lists);
        // Once submitted, no resource may retire without a fence or device loss.
        phase=Phase::FailedSubmission;hr=transferQueue->Signal(fence.Get(),++fenceValue);if(FAILED(hr))return hr;
        phase=Phase::Gpu;ResetEvent(gpuEvent);hr=fence->SetEventOnCompletion(fenceValue,gpuEvent);
        // A failed event registration is recoverable through fence polling.
        if(FAILED(hr))failure=hr;return S_OK;
    }
public:
    PackedTextureBatch()=default;PackedTextureBatch(const PackedTextureBatch&)=delete;PackedTextureBatch& operator=(const PackedTextureBatch&)=delete;
    Phase CurrentPhase()const{return phase;}
    const InitTiming& InitializationTiming()const{return initTiming;}
    UINT64 BufferAllocations()const{return bufferAllocations;}
    UINT64 ReusedBatches()const{return reusedBatches;}
    UINT64 RetainedBytes()const{return packedCapacity+paddedCapacity;}
    HANDLE Event()const{return phase==Phase::Storage?storageEvent:gpuEvent;}
    HRESULT Initialize(IDStorageFactory* factory,ID3D12CommandQueue* queue,const PackedTextureBatch* shared=nullptr){
        if(!factory||!queue||device||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return E_INVALIDARG;
        initTiming={};UINT64 start=NowUs();graphics=queue;HRESULT hr=queue->GetDevice(IID_PPV_ARGS(&device));if(FAILED(hr))return hr;
        // Loading can wait for GPU jobs while the renderer has unsubmitted lists.
        // An owned queue avoids making those jobs depend on future frame submits.
        D3D12_COMMAND_QUEUE_DESC uploadQueue{};uploadQueue.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
        hr=device->CreateCommandQueue(&uploadQueue,IID_PPV_ARGS(&transferQueue));if(FAILED(hr))return hr;
        DSTORAGE_QUEUE_DESC d{};d.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;d.Device=device.Get();d.Capacity=128;d.Priority=DSTORAGE_PRIORITY_NORMAL;d.Name="ControlFG native packed textures";
        hr=factory->CreateQueue(&d,IID_PPV_ARGS(&storage));if(FAILED(hr))return hr;
        hr=factory->CreateStatusArray(1,"ControlFG packed batch",IID_PPV_ARGS(&status));if(FAILED(hr))return hr;
        initTiming.queuesUs=NowUs()-start;start=NowUs();
        if(shared){
            if(!shared->root||!shared->pipeline||!Same(device.Get(),shared->device.Get()))return E_INVALIDARG;
            root=shared->root;pipeline=shared->pipeline;initTiming.sharedPipeline=true;
        }else{
            hr=device->CreateRootSignature(0,ControlFGPackedRoot,sizeof(ControlFGPackedRoot),IID_PPV_ARGS(&root));if(FAILED(hr))return hr;
            D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={ControlFGPackedShader,sizeof(ControlFGPackedShader)};
            hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline));if(FAILED(hr))return hr;
        }
        initTiming.pipelineUs=NowUs()-start;start=NowUs();
        hr=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));if(FAILED(hr))return hr;
        hr=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pipeline.Get(),IID_PPV_ARGS(&list));if(FAILED(hr))return hr;
        hr=list->Close();if(FAILED(hr))return hr;hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));if(FAILED(hr))return hr;
        storageEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);gpuEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        initTiming.commandsUs=NowUs()-start;return storageEvent&&gpuEvent?S_OK:HRESULT_FROM_WIN32(GetLastError());
    }
    HRESULT Submit(const Input* inputs,unsigned n){
        if(!storage||phase!=Phase::Idle||!inputs||!n||n>Capacity)return E_INVALIDARG;
        DSTORAGE_REQUEST requests[Capacity]{};UINT64 packedBytes=0,paddedBytes=0;
        for(unsigned j=0;j<n;++j){const auto& in=inputs[j];auto& i=items[j];HRESULT hr=E_INVALIDARG;
            if(!in.file||!in.texture||!in.bytes||in.bytes>32u*1024u*1024u){Clear();return hr;}
            for(unsigned k=0;k<j;++k)if(Same(in.texture,inputs[k].texture)){Clear();return hr;}
            Ptr<ID3D12Device> owner;if(FAILED(in.texture->GetDevice(IID_PPV_ARGS(&owner)))||!Same(owner.Get(),device.Get())){Clear();return hr;}
            D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
            if(FAILED(in.texture->GetHeapProperties(&heap,&flags))||heap.Type!=D3D12_HEAP_TYPE_DEFAULT){Clear();return hr;}
            const auto desc=in.texture->GetDesc();
            if(!native_texture_layout::Build(device.Get(),desc,i.layout)||i.layout.compactBytes!=in.bytes){Clear();return hr;}
            for(unsigned m=0;m<i.layout.count;++m)if(i.layout.mips[m].rowBytes%4||i.layout.mips[m].rows>65535){Clear();return hr;}
            BY_HANDLE_FILE_INFORMATION info{};hr=in.file->GetFileInformation(&info);const UINT64 length=(UINT64(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
            if(FAILED(hr)||in.offset>length||in.bytes>length-in.offset){Clear();return E_INVALIDARG;}
            device->GetCopyableFootprints(&desc,0,i.layout.count,0,i.footprints,nullptr,nullptr,&i.paddedBytes);
            i.packedOffset=Aligned(packedBytes,512);packedBytes=i.packedOffset+in.bytes;
            const UINT64 paddedOffset=Aligned(paddedBytes,512);paddedBytes=paddedOffset+i.paddedBytes;
            if(packedBytes+paddedBytes>StagingBudget){Clear();return E_OUTOFMEMORY;}
            for(unsigned m=0;m<i.layout.count;++m)i.footprints[m].Offset+=paddedOffset;
            i.file=in.file;i.target=in.texture;i.state=in.state;
        }
        const HRESULT prepared=EnsureArenas(packedBytes,paddedBytes);if(FAILED(prepared)){Clear();return prepared;}
        for(unsigned j=0;j<n;++j){const auto& in=inputs[j];auto& i=items[j];i.packed=packedArena;i.padded=paddedArena;
            auto& r=requests[j];r.Options.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;r.Options.DestinationType=DSTORAGE_REQUEST_DESTINATION_BUFFER;
            r.Options.CompressionFormat=DSTORAGE_COMPRESSION_FORMAT_NONE;r.Source.File={in.file,in.offset,in.bytes};r.Destination.Buffer={i.packed.Get(),i.packedOffset,in.bytes};r.UncompressedSize=in.bytes;
        }
        count=n;failure=S_OK;ResetEvent(storageEvent);
        for(unsigned j=0;j<n;++j)storage->EnqueueRequest(&requests[j]);storage->EnqueueStatus(status.Get(),0);storage->EnqueueSetEvent(storageEvent);phase=Phase::Storage;storage->Submit();return S_OK;
    }
    // true means the whole batch has retired, with destinations restored to the
    // supplied state on success. No destination is published by this class.
    bool Poll(HRESULT& result){
        if(phase==Phase::Idle)return false;
        if(phase==Phase::Storage){
            if(WaitForSingleObject(storageEvent,0)!=WAIT_OBJECT_0)return false;
            result=status->GetHResult(0);if(FAILED(result)){Clear();return true;}
            result=Upload();if(FAILED(result)){failure=result;if(phase!=Phase::FailedSubmission){Clear();return true;}}
        }
        if(phase==Phase::Gpu){const auto done=fence->GetCompletedValue();if(done==UINT64_MAX){result=DXGI_ERROR_DEVICE_REMOVED;Clear();return true;}
            if(done<fenceValue)return false;
            // Queue the explicit consumer dependency before native publication.
            // The fence is already complete, so this does not wait for CPU work.
            result=graphics->Wait(fence.Get(),fenceValue);Clear();return true;}
        if(phase==Phase::FailedSubmission){
            if(FAILED(device->GetDeviceRemovedReason())){result=DXGI_ERROR_DEVICE_REMOVED;Clear();return true;}
            // Recover a transient Signal failure without retiring submitted work.
            if(SUCCEEDED(transferQueue->Signal(fence.Get(),fenceValue))){phase=Phase::Gpu;ResetEvent(gpuEvent);fence->SetEventOnCompletion(fenceValue,gpuEvent);}
        }
        return false;
    }
    bool Close(){if(phase!=Phase::Idle)return false;Clear();packedArena.Reset();paddedArena.Reset();packedCapacity=paddedCapacity=0;list.Reset();allocator.Reset();pipeline.Reset();root.Reset();storage.Reset();status.Reset();fence.Reset();transferQueue.Reset();graphics.Reset();device.Reset();
        if(storageEvent)CloseHandle(storageEvent);if(gpuEvent)CloseHandle(gpuEvent);storageEvent=gpuEvent=nullptr;return true;}
};
}
