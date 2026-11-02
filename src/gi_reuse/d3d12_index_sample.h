#pragma once
#include "d3d12_index_snapshot.h"
#include "d3d12_storage_lease.h"
#include <memory>
namespace gi_reuse {
// A diagnostic owns everything needed by its recorded commands until the host
// seals the recording and proves all submissions complete. No native lighting use.
struct IndexSample {
    Microsoft::WRL::ComPtr<ID3D12Resource> source,output,readback;
    ComputeBindingSnapshot bindings;
    IndexSnapshotPipeline retainedPipeline;
    IndexSnapshotPlan plan;
    bool recorded=false;
    std::uint64_t offset=0,bytes=0,fullBytes=0,charge=0;
    std::uint32_t vertexCount=0,rawReject=0;
    static constexpr std::uint64_t MaxSampleBytes=256*1024;
    static D3D12_RESOURCE_DESC Description(std::uint64_t size,D3D12_RESOURCE_FLAGS flags) noexcept {
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;return d;
    }
    static std::uint64_t Charge(ID3D12Device* device,ID3D12Resource* source,UINT outputBytes) noexcept {
        if(!device||!source)return 0;const auto src=source->GetDesc();
        const auto out=Description(outputBytes,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        const auto read=Description(outputBytes,D3D12_RESOURCE_FLAG_NONE);
        std::uint64_t sum=0;for(const auto* d:{&src,&out,&read}){
            const auto size=device->GetResourceAllocationInfo(0,1,d).SizeInBytes;
            if(!size||size==UINT64_MAX||size>UINT64_MAX-sum)return 0;sum+=size;
        }return sum;
    }
    HRESULT Prepare(ID3D12Device* device,ID3D12Resource* input,std::uint64_t inputOffset,std::uint64_t inputBytes,
                    const ComputeBindingSnapshot& native) noexcept {
        if(!device||!input||!native.Ready()||inputBytes>MaxSampleBytes)return E_INVALIDARG;
        BufferStorageLease lease;const auto desc=input->GetDesc();D3D12_HEAP_PROPERTIES properties{};D3D12_HEAP_FLAGS flags{};
        if(!AcquireStorageLease(input,lease)||lease.kind!=BufferStorageKind::Committed||
           FAILED(input->GetHeapProperties(&properties,&flags))||properties.Type!=D3D12_HEAP_TYPE_DEFAULT||
           desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||(desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)||
           !IndexSnapshotPlan::Make(desc.Width,inputOffset,inputBytes,plan))return E_INVALIDARG;
        charge=Charge(device,input,plan.copyBytes);if(!charge)return E_INVALIDARG;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        auto outputDesc=Description(plan.copyBytes,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&outputDesc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&output));if(FAILED(hr))return hr;
        heap.Type=D3D12_HEAP_TYPE_READBACK;auto readDesc=Description(plan.copyBytes,D3D12_RESOURCE_FLAG_NONE);
        hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&readDesc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback));if(FAILED(hr))return hr;
        source=input;offset=inputOffset;bytes=inputBytes;bindings=native;return S_OK;
    }
    bool Record(ID3D12GraphicsCommandList4* list,const IndexSnapshotPipeline& pipeline) noexcept {
        if(!source||!output||!readback)return false;IndexSnapshotPlan recorded;
        retainedPipeline=pipeline;
        if(!pipeline.RecordRestoring(list,source.Get(),offset,bytes,output.Get(),bindings,recorded))return false;
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={output.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
        list->CopyBufferRegion(readback.Get(),0,output.Get(),0,recorded.copyBytes);
        // Leave the private output in its initial state for legal closed-list replay.
        std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);recorded=true;return true;
    }
    struct Summary {std::uint64_t hash=14695981039346656037ull;UINT maximum=0,outOfRange=0;bool paddingZero=true;};
    // Caller must prove sealed recording + GPU completion before this CPU read.
    HRESULT Read(Summary& result) const noexcept {
        result={};if(!readback)return E_INVALIDARG;void* mapped=nullptr;D3D12_RANGE range{0,plan.copyBytes};
        auto hr=readback->Map(0,&range,&mapped);if(FAILED(hr))return hr;
        const auto* p=static_cast<const unsigned char*>(mapped);
        for(std::uint64_t i=0;i<bytes;++i){result.hash^=p[i];result.hash*=1099511628211ull;}
        for(std::uint64_t i=0;i<bytes;i+=2){const UINT index=UINT(p[i])|(UINT(p[i+1])<<8);if(index>result.maximum)result.maximum=index;if(vertexCount&&index>=vertexCount)++result.outOfRange;}
        for(std::uint64_t i=bytes;i<plan.copyBytes;++i)if(p[i])result.paddingZero=false;
        D3D12_RANGE none{0,0};readback->Unmap(0,&none);return S_OK;
    }
};
}
