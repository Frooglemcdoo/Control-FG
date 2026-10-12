#pragma once
#include "pt_scene_pipeline.h"
#include "pt_scene_leases.h"
// A complete, immutable private SBT prepared on the CPU before recording rays.
// Native asynchronous writer pointers are never referenced by this object.
namespace PTScenePipeline {
class Dispatch {
    std::shared_ptr<Pipeline> pipeline;
    Microsoft::WRL::ComPtr<ID3D12Resource> table;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DISPATCH_RAYS_DESC rays{};
    std::vector<std::shared_ptr<void>> geometry;
public:
    static HRESULT Create(ID3D12Device* device,const std::shared_ptr<Pipeline>& p,const PTSceneDispatch::Plan& plan,
                          UINT width,UINT height,ID3D12DescriptorHeap* descriptors,std::shared_ptr<Dispatch>& output){
        output.reset();
        if(!device||!p||!p->State()||!descriptors||!width||!height||std::uint64_t(width)*height>(1ull<<30)||
           plan.privatePipeline!=reinterpret_cast<UINT_PTR>(p->State())||plan.records.empty()||plan.records.size()>131072)return E_INVALIDARG;
        const auto heapDesc=descriptors->GetDesc();
        if(heapDesc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(heapDesc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))return E_INVALIDARG;
        auto pending=std::make_shared<Dispatch>();pending->pipeline=p;pending->heap=descriptors;pending->geometry=plan.keepAlive;
        const UINT64 hitBytes=UINT64(plan.records.size())*Pipeline::HitStride,totalBytes=128+hitBytes;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=totalBytes;desc.Height=1;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        HRESULT hr=device->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&pending->table));
        if(FAILED(hr))return hr;
        void* mapped=nullptr;const D3D12_RANGE none{};hr=pending->table->Map(0,&none,&mapped);if(FAILED(hr))return hr;
        auto* data=static_cast<unsigned char*>(mapped);std::memset(data,0,128);
        std::memcpy(data,p->RayGeneration().data(),32);std::memcpy(data+64,p->SurfaceMiss().data(),32);std::memcpy(data+96,p->ShadowMiss().data(),32);
        static_assert(sizeof(PTShaderTableAdapter::Bytes)==Pipeline::HitStride,"private hit-table stride");
        std::memcpy(data+128,plan.records.data(),static_cast<std::size_t>(hitBytes));
        const D3D12_RANGE written{0,static_cast<SIZE_T>(totalBytes)};pending->table->Unmap(0,&written);
        const auto address=pending->table->GetGPUVirtualAddress();
        pending->rays.RayGenerationShaderRecord={address,32};pending->rays.MissShaderTable={address+64,64,32};
        pending->rays.HitGroupTable={address+128,hitBytes,Pipeline::HitStride};
        pending->rays.Width=width;pending->rays.Height=height;pending->rays.Depth=1;
        output=std::move(pending);return S_OK;
    }
    const D3D12_DISPATCH_RAYS_DESC& Description()const{return rays;}
    // Caller has verified the pass/root, exact submission scene and descriptor
    // contents, and attached this object to its fence-owned scene lease before
    // calling Record. The existing native root and descriptor heaps stay bound.
    bool Record(ID3D12GraphicsCommandList4* list,ID3D12RootSignature* nativeRoot,ID3D12PipelineState* nativeCompute,
                D3D12_GPU_DESCRIPTOR_HANDLE nativeTable,D3D12_GPU_DESCRIPTOR_HANDLE privateTable){
        if(!list||!table||!pipeline||nativeRoot!=pipeline->Root()||!nativeCompute||!nativeTable.ptr||!privateTable.ptr)return false;
        if(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE)return false;
        const auto base=heap->GetGPUDescriptorHandleForHeapStart().ptr;
        Microsoft::WRL::ComPtr<ID3D12Device> device;if(FAILED(heap->GetDevice(IID_PPV_ARGS(&device))))return false;
        const auto step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const auto count=heap->GetDesc().NumDescriptors;
        if(!step||count<49)return false;
        const auto inside=[&](UINT64 address){return address>=base&&(address-base)%step==0&&(address-base)/step<=count-49;};
        if(!inside(nativeTable.ptr)||!inside(privateTable.ptr))return false;
        const auto nativeIndex=(nativeTable.ptr-base)/step,privateIndex=(privateTable.ptr-base)/step;
        if(nativeIndex<privateIndex+49&&privateIndex<nativeIndex+49)return false;
        list->SetPipelineState1(pipeline->State());list->SetComputeRootDescriptorTable(0,privateTable);list->DispatchRays(&rays);
        list->SetComputeRootDescriptorTable(0,nativeTable);list->SetPipelineState(nativeCompute);return true;
    }
};
}
