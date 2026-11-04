#pragma once
#include "producer_table.h"
#include "d3d12_storage_lease.h"
#include "producer_backing_ranges.h"
#include "d3d12_gi_scratch_pool.h"
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
    bool deferred_=false,pooled_=false;
public:
    D3D12ProducerTable()=default;
    // The enclosing pass remains retained by recordings/flights through GPU
    // completion. Return storage only when that final owner is destroyed.
    ~D3D12ProducerTable(){if(pooled_)SharedGIScratchPool().Return(std::move(upload_),D3D12_HEAP_TYPE_UPLOAD);}
    D3D12ProducerTable(const D3D12ProducerTable&)=delete;
    D3D12ProducerTable& operator=(const D3D12ProducerTable&)=delete;
private:
    HRESULT InitializeImpl(ID3D12Device* device,ID3D12StateObject* pipeline,
        const GIProducerTable& plan,const std::vector<ID3D12Resource*>& resources,bool validationOnly) noexcept {
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
            // Deferred commit already owns its upload. Keep the exact validation
            // and leases, without allocating/writing a second temporary table.
            if(validationOnly){backing_=std::move(leases);retainedBytes_=retained;return S_OK;}
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
public:
    HRESULT Initialize(ID3D12Device* device,ID3D12StateObject* pipeline,
        const GIProducerTable& plan,const std::vector<ID3D12Resource*>& resources) noexcept {
        return InitializeImpl(device,pipeline,plan,resources,false);
    }
    // Reserve the exact writer row count while recording. Every row is zero;
    // raygen must remain disarmed until Commit succeeds before first submission.
    HRESULT Reserve(ID3D12Device* device,ID3D12StateObject* pipeline,UINT bindings,UINT64 epoch,UINT64 version) noexcept {
        if(upload_||!device||!pipeline||!bindings||bindings>65536||!epoch||!version)return E_INVALIDARG;
        const UINT64 bytes=UINT64(bindings)*2*PTShaderTableAdapter::PrivateStride;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;desc.Height=1;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Ptr<ID3D12Resource> upload;
        try {upload=SharedGIScratchPool().Take(device,bytes,D3D12_HEAP_TYPE_UPLOAD);}catch(...){}
        HRESULT hr=S_OK;
        if(!upload)hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload));
        if(FAILED(hr))return hr;
        void* mapped=nullptr;const D3D12_RANGE empty{};hr=upload->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
        if(!mapped){upload->Unmap(0,&empty);return E_UNEXPECTED;}
        std::memset(mapped,0,static_cast<SIZE_T>(bytes));const D3D12_RANGE written{0,static_cast<SIZE_T>(bytes)};upload->Unmap(0,&written);
        pipeline_=pipeline;upload_=std::move(upload);epoch_=epoch;version_=version;size_=retainedBytes_=bytes;deferred_=true;pooled_=true;return S_OK;
    }
    // Host calls once at the serialized first Execute boundary, after selecting
    // the strict scene cut and before arming the recorded constant upload. Never
    // call on an in-flight/replayed command list. Failure leaves zero records.
    HRESULT Commit(ID3D12Device* device,const GIProducerTable& plan,const std::vector<ID3D12Resource*>& resources) noexcept {
        if(!deferred_||!upload_||plan.epoch!=epoch_||plan.writerVersion!=version_||
           plan.records.size()*PTShaderTableAdapter::PrivateStride!=size_)return E_INVALIDARG;
        Ptr<ID3D12Device> owner;if(!device||FAILED(upload_->GetDevice(IID_PPV_ARGS(&owner)))||owner.Get()!=device)return E_INVALIDARG;
        D3D12ProducerTable checked;auto hr=checked.InitializeImpl(device,pipeline_.Get(),plan,resources,true);if(FAILED(hr))return hr;
        void* mapped=nullptr;const D3D12_RANGE empty{};hr=upload_->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
        if(!mapped){upload_->Unmap(0,&empty);return E_UNEXPECTED;}
        std::memcpy(mapped,plan.records.data(),static_cast<SIZE_T>(size_));const D3D12_RANGE written{0,static_cast<SIZE_T>(size_)};upload_->Unmap(0,&written);
        backing_=std::move(checked.backing_);retainedBytes_=checked.retainedBytes_;deferred_=false;return S_OK;
    }
    bool Deferred() const noexcept{return deferred_;}
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
