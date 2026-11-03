#pragma once
#include "producer_table.h"
#include "d3d12_storage_lease.h"
#include "producer_backing_ranges.h"
#include <cstring>
#include <unordered_set>
namespace gi_reuse {
// An immutable GPU table and its backing resource leases. Store this owner in
// D3D12SnapshotRetirement before recording traversal; submission completion,
// alias activation and scene-version revalidation belong to that host path.
class D3D12ProducerTable {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12StateObject> pipeline_;
    Ptr<ID3D12Resource> upload_;
    std::vector<BufferStorageLease> backing_;
    std::uint64_t epoch_=0,version_=0,retainedBytes_=0;
    UINT64 size_=0;
public:
    D3D12ProducerTable()=default;
    D3D12ProducerTable(const D3D12ProducerTable&)=delete;
    D3D12ProducerTable& operator=(const D3D12ProducerTable&)=delete;
    HRESULT Initialize(ID3D12Device* device,ID3D12StateObject* pipeline,
        const GIProducerTable& plan,const std::vector<ID3D12Resource*>& resources) noexcept {
        if(upload_||!device||!pipeline||!plan.epoch||!plan.writerVersion||!plan.nativePipeline||
           plan.producerPipeline!=reinterpret_cast<std::uintptr_t>(pipeline)||
           plan.geometry.empty()||plan.geometry.size()>65536||
           plan.bound.size()!=plan.geometry.size()||
           plan.records.size()!=plan.geometry.size()*2||resources.empty()||resources.size()>196608)return E_INVALIDARG;
        try {
            std::vector<BufferStorageLease> leases;
            leases.reserve(resources.size());
            std::unordered_set<ID3D12Resource*> unique;
            unique.reserve(resources.size());
            ProducerBackingRanges ranges;
            std::uint64_t retained=plan.records.size()*PTShaderTableAdapter::PrivateStride;
            for(auto* resource:resources){
                if(!resource)return E_INVALIDARG;
                if(!unique.insert(resource).second)continue;
                BufferStorageLease lease;if(!AcquireStorageLease(resource,lease))return E_INVALIDARG;
                const auto desc=resource->GetDesc();const auto bytes=desc.Width;
                if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||
                   !ranges.Add(resource->GetGPUVirtualAddress(),bytes))return E_INVALIDARG;
                if(bytes>UINT64_MAX-retained)return E_INVALIDARG;
                retained+=bytes;leases.push_back(std::move(lease));
            }
            ranges.Seal();
            for(std::size_t i=0;i<plan.geometry.size();++i){
                const auto& geometry=plan.geometry[i];
                const PTShaderTableAdapter::Bytes zero{};
                if(plan.records[i*2+1]!=zero)return E_INVALIDARG;
                if(!plan.bound[i]){
                    if(geometry.epoch||plan.records[i*2]!=zero)return E_INVALIDARG;
                    continue;
                }
                if(plan.bound[i]!=1)return E_INVALIDARG;
                if(geometry.epoch!=plan.epoch)return E_INVALIDARG;
                PTSceneAdmission::Record checked;
                if(PTSceneAdmission::Admit(geometry,checked)!=PTSceneAdmission::Error::none)return E_INVALIDARG;
                for(const auto view:{geometry.indices,geometry.positions,geometry.attributes}){
                    if(!ranges.Unique(view.allocation.address,view.allocation.bytes))return E_INVALIDARG;
                }
            }
            const UINT64 bytes=plan.records.size()*PTShaderTableAdapter::PrivateStride;
            D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
            D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;desc.Height=1;
            desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Ptr<ID3D12Resource> upload;
            HRESULT hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload));
            if(FAILED(hr))return hr;
            if(!upload->GetGPUVirtualAddress()||(upload->GetGPUVirtualAddress()%D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT))return E_UNEXPECTED;
            void* data=nullptr;const D3D12_RANGE empty{};
            hr=upload->Map(0,&empty,&data);if(FAILED(hr))return hr;
            if(!data){upload->Unmap(0,&empty);return E_UNEXPECTED;}
            std::memcpy(data,plan.records.data(),static_cast<std::size_t>(bytes));
            const D3D12_RANGE written{0,static_cast<SIZE_T>(bytes)};upload->Unmap(0,&written);
            pipeline_=pipeline;upload_=std::move(upload);backing_=std::move(leases);
            epoch_=plan.epoch;version_=plan.writerVersion;size_=bytes;retainedBytes_=retained;return S_OK;
        }catch(...){return E_OUTOFMEMORY;}
    }
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Range() const noexcept {
        return upload_?D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{upload_->GetGPUVirtualAddress(),size_,PTShaderTableAdapter::PrivateStride}:
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{};
    }
    ID3D12StateObject* Pipeline() const noexcept {return pipeline_.Get();}
    std::uint64_t Epoch() const noexcept {return epoch_;}
    std::uint64_t WriterVersion() const noexcept {return version_;}
    std::uint64_t RetainedBytes() const noexcept {return retainedBytes_;}
};
}
