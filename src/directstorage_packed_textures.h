#pragma once
#include "native_texture_layout.h"
#include <windows.h>
#include <dstorage.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>
namespace control_storage {
// One event-driven batch owner. Payloads are never mapped by the CPU.
// Caller holds exclusive engine ownership of each unpublished destination until
// Poll completes, and supplies its actual before/after state on this direct queue.
class PackedTextureBatch {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
public:
    static constexpr unsigned Capacity=16;
    struct Input {IDStorageFile* file=nullptr;ID3D12Resource* texture=nullptr;UINT64 offset=0;UINT bytes=0;D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON;};
    enum class Phase {Idle,Storage,Gpu,FailedSubmission};
private:
    struct Item {
        Ptr<IDStorageFile> file;Ptr<ID3D12Resource> target,packed,padded;
        native_texture_layout::Layout layout{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[16]{};
        D3D12_RESOURCE_STATES state{};UINT64 paddedBytes=0;
    } items[Capacity];
    Ptr<ID3D12Device> device;Ptr<ID3D12CommandQueue> graphics;Ptr<IDStorageQueue2> storage;
    Ptr<IDStorageStatusArray> status;Ptr<ID3D12RootSignature> root;Ptr<ID3D12PipelineState> pipeline;
    Ptr<ID3D12CommandAllocator> allocator;Ptr<ID3D12GraphicsCommandList> list;Ptr<ID3D12Fence> fence;
    HANDLE storageEvent=nullptr,gpuEvent=nullptr;UINT64 fenceValue=0;unsigned count=0;Phase phase=Phase::Idle;
    HRESULT failure=S_OK;
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
        for(unsigned j=0;j<count;++j){auto& i=items[j];
            Barrier(i.packed.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            Barrier(i.padded.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            list->SetComputeRootShaderResourceView(0,i.packed->GetGPUVirtualAddress());list->SetComputeRootUnorderedAccessView(1,i.padded->GetGPUVirtualAddress());
            for(unsigned m=0;m<i.layout.count;++m){const auto& l=i.layout.mips[m];const auto& fp=i.footprints[m];
                const UINT data[]={UINT(l.compactOffset),UINT(fp.Offset),UINT(l.rowBytes/4),l.rows,fp.Footprint.RowPitch/4};
                list->SetComputeRoot32BitConstants(2,5,data,0);list->Dispatch((data[2]+63)/64,data[3],1);
            }
            Barrier(i.padded.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
            Barrier(i.target.Get(),i.state,D3D12_RESOURCE_STATE_COPY_DEST);
            for(unsigned m=0;m<i.layout.count;++m){D3D12_TEXTURE_COPY_LOCATION dst{},src{};
                dst.pResource=i.target.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=m;
                src.pResource=i.padded.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=i.footprints[m];
                list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            }
            Barrier(i.target.Get(),D3D12_RESOURCE_STATE_COPY_DEST,i.state);
        }
        hr=list->Close();if(FAILED(hr))return hr;
        ID3D12CommandList* lists[]={list.Get()};graphics->ExecuteCommandLists(1,lists);
        // Once submitted, no resource may retire without a fence or device loss.
        phase=Phase::FailedSubmission;hr=graphics->Signal(fence.Get(),++fenceValue);if(FAILED(hr))return hr;
        phase=Phase::Gpu;ResetEvent(gpuEvent);hr=fence->SetEventOnCompletion(fenceValue,gpuEvent);
        // A failed event registration is recoverable through fence polling.
        if(FAILED(hr))failure=hr;return S_OK;
    }
public:
    PackedTextureBatch()=default;PackedTextureBatch(const PackedTextureBatch&)=delete;PackedTextureBatch& operator=(const PackedTextureBatch&)=delete;
    Phase CurrentPhase()const{return phase;}
    HANDLE Event()const{return phase==Phase::Storage?storageEvent:gpuEvent;}
    HRESULT Initialize(IDStorageFactory* factory,ID3D12CommandQueue* queue){
        if(!factory||!queue||device||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return E_INVALIDARG;
        graphics=queue;HRESULT hr=queue->GetDevice(IID_PPV_ARGS(&device));if(FAILED(hr))return hr;
        DSTORAGE_QUEUE_DESC d{};d.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;d.Device=device.Get();d.Capacity=128;d.Priority=DSTORAGE_PRIORITY_NORMAL;d.Name="ControlFG native packed textures";
        hr=factory->CreateQueue(&d,IID_PPV_ARGS(&storage));if(FAILED(hr))return hr;
        hr=factory->CreateStatusArray(1,"ControlFG packed batch",IID_PPV_ARGS(&status));if(FAILED(hr))return hr;
        D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants.Num32BitValues=5;
        D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=params;Ptr<ID3DBlob> signature,error,shader;
        hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&signature,&error);if(FAILED(hr))return hr;
        hr=device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&root));if(FAILED(hr))return hr;
        const char source[]=R"(ByteAddressBuffer inputBytes : register(t0); RWByteAddressBuffer outputBytes : register(u0);
cbuffer Layout : register(b0) { uint srcOffset; uint dstOffset; uint rowWords; uint rows; uint pitchWords; };
[numthreads(64,1,1)] void main(uint3 id:SV_DispatchThreadID) {
 if(id.x<rowWords && id.y<rows) outputBytes.Store(dstOffset+4*(id.y*pitchWords+id.x),inputBytes.Load(srcOffset+4*(id.y*rowWords+id.x)));
})";
        hr=D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&shader,&error);if(FAILED(hr))return hr;
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};
        hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline));if(FAILED(hr))return hr;
        hr=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));if(FAILED(hr))return hr;
        hr=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pipeline.Get(),IID_PPV_ARGS(&list));if(FAILED(hr))return hr;
        hr=list->Close();if(FAILED(hr))return hr;hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));if(FAILED(hr))return hr;
        storageEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);gpuEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        return storageEvent&&gpuEvent?S_OK:HRESULT_FROM_WIN32(GetLastError());
    }
    HRESULT Submit(const Input* inputs,unsigned n){
        if(!storage||phase!=Phase::Idle||!inputs||!n||n>Capacity)return E_INVALIDARG;
        DSTORAGE_REQUEST requests[Capacity]{};UINT64 memory=0;
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
            memory+=i.paddedBytes+in.bytes;if(i.paddedBytes>UINT_MAX||memory>256ull*1024*1024){Clear();return E_OUTOFMEMORY;}
            hr=Buffer(in.bytes,D3D12_RESOURCE_FLAG_NONE,&i.packed);if(FAILED(hr)){Clear();return hr;}
            hr=Buffer(i.paddedBytes,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,&i.padded);if(FAILED(hr)){Clear();return hr;}
            i.file=in.file;i.target=in.texture;i.state=in.state;
            auto& r=requests[j];r.Options.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;r.Options.DestinationType=DSTORAGE_REQUEST_DESTINATION_BUFFER;
            r.Options.CompressionFormat=DSTORAGE_COMPRESSION_FORMAT_NONE;r.Source.File={in.file,in.offset,in.bytes};r.Destination.Buffer={i.packed.Get(),0,in.bytes};r.UncompressedSize=in.bytes;
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
            if(done<fenceValue)return false;result=S_OK;Clear();return true;}
        if(phase==Phase::FailedSubmission&&FAILED(device->GetDeviceRemovedReason())){result=failure;Clear();return true;}
        return false;
    }
    bool Close(){if(phase!=Phase::Idle)return false;Clear();list.Reset();allocator.Reset();pipeline.Reset();root.Reset();storage.Reset();status.Reset();fence.Reset();graphics.Reset();device.Reset();
        if(storageEvent)CloseHandle(storageEvent);if(gpuEvent)CloseHandle(gpuEvent);storageEvent=gpuEvent=nullptr;return true;}
};
}
