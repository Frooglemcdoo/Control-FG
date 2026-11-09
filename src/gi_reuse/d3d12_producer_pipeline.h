#pragma once
#include "producer_hit.h"
#include "pt_shader_table_adapter.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
namespace gi_reuse {
// Uses a hash-admitted GI6 global root and private descriptor table. Owns the
// enlarged payload/local root; never mutates native state objects or tables.
class D3D12ProducerPipeline {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12RootSignature> global_,local_;
    Ptr<ID3D12StateObject> state_;
    Ptr<ID3D12Resource> records_;
    PTShaderTableAdapter::Identifier opaque_{},alpha_{};
public:
    HRESULT Initialize(ID3D12Device5* device,ID3D12RootSignature* admittedGlobal,
                       D3D12_SHADER_BYTECODE library) noexcept {
        if(state_||!device||!admittedGlobal||!library.pShaderBytecode||!library.BytecodeLength)return E_INVALIDARG;
        Ptr<ID3D12RootSignature> local;
        D3D12_ROOT_PARAMETER parameters[4]{};
        for(UINT i=0;i<3;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;parameters[i].Descriptor={i,3};}
        parameters[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[3].Constants={0,3,12};
        D3D12_ROOT_SIGNATURE_DESC rd{4,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE};
        Ptr<ID3DBlob> blob,error;
        HRESULT hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);
        if(FAILED(hr))return hr;
        hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&local));
        if(FAILED(hr))return hr;
        const D3D12_EXPORT_DESC exports[]={
            {L"giProduceAcceptedHits",nullptr,D3D12_EXPORT_FLAG_NONE},
            {L"giAcceptedGeometry",nullptr,D3D12_EXPORT_FLAG_NONE},
            {L"giAcceptedAlpha",nullptr,D3D12_EXPORT_FLAG_NONE},
            {L"giGeometryMiss",nullptr,D3D12_EXPORT_FLAG_NONE}};
        const D3D12_DXIL_LIBRARY_DESC dxil{library,4,exports};
        const D3D12_HIT_GROUP_DESC opaque{L"giProducerOpaque",D3D12_HIT_GROUP_TYPE_TRIANGLES,nullptr,L"giAcceptedGeometry",nullptr};
        const D3D12_HIT_GROUP_DESC alpha{L"giProducerAlpha",D3D12_HIT_GROUP_TYPE_TRIANGLES,L"giAcceptedAlpha",L"giAcceptedGeometry",nullptr};
        const D3D12_GLOBAL_ROOT_SIGNATURE globalDesc{admittedGlobal};
        const D3D12_LOCAL_ROOT_SIGNATURE localDesc{local.Get()};
        const D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{sizeof(GIProducerHit),8};
        const D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{1};
        D3D12_STATE_SUBOBJECT subobjects[8]={
            {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&dxil},
            {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&opaque},
            {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&alpha},
            {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&globalDesc},
            {D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&localDesc},
            {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&shaderConfig},
            {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipelineConfig},
            {}};
        LPCWSTR localExports[]={L"giProducerOpaque",L"giProducerAlpha"};
        const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{subobjects+4,2,localExports};
        subobjects[7]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&association};
        const D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,8,subobjects};
        Ptr<ID3D12StateObject> state;
        hr=device->CreateStateObject(&desc,IID_PPV_ARGS(&state));if(FAILED(hr))return hr;
        Ptr<ID3D12StateObjectProperties> properties;hr=state.As(&properties);if(FAILED(hr))return hr;
        const void* raygen=properties->GetShaderIdentifier(L"giProduceAcceptedHits");
        const void* miss=properties->GetShaderIdentifier(L"giGeometryMiss");
        const void* opaqueId=properties->GetShaderIdentifier(L"giProducerOpaque");
        const void* alphaId=properties->GetShaderIdentifier(L"giProducerAlpha");
        if(!raygen||!miss||!opaqueId||!alphaId)return E_UNEXPECTED;
        std::array<unsigned char,128> bytes{};
        std::memcpy(bytes.data(),raygen,32);std::memcpy(bytes.data()+64,miss,32);
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes.size();buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Ptr<ID3D12Resource> records;
        hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&records));
        if(FAILED(hr))return hr;
        void* mapped=nullptr;const D3D12_RANGE empty{};hr=records->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
        if(!mapped){records->Unmap(0,&empty);return E_UNEXPECTED;}
        std::memcpy(mapped,bytes.data(),bytes.size());const D3D12_RANGE written{0,bytes.size()};records->Unmap(0,&written);
        std::memcpy(opaque_.data(),opaqueId,32);std::memcpy(alpha_.data(),alphaId,32);
        global_=admittedGlobal;local_=std::move(local);state_=std::move(state);records_=std::move(records);return S_OK;
    }
    ID3D12StateObject* State() const noexcept {return state_.Get();}
    ID3D12RootSignature* Root() const noexcept {return global_.Get();}
    const PTShaderTableAdapter::Identifier& Opaque() const noexcept {return opaque_;}
    const PTShaderTableAdapter::Identifier& Alpha() const noexcept {return alpha_;}
    // The host owns the admitted hit-table storage and its geometry/scene lease.
    bool DispatchDescription(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE hits,UINT count,
                             D3D12_DISPATCH_RAYS_DESC& output) const noexcept {
        output={};
        if(!records_||!count||count>1073741824u||!hits.StartAddress||hits.StartAddress%64||
           hits.StrideInBytes!=PTShaderTableAdapter::PrivateStride||!hits.SizeInBytes||
           hits.SizeInBytes%(2*PTShaderTableAdapter::PrivateStride)||
           hits.SizeInBytes>65536ull*2*PTShaderTableAdapter::PrivateStride)return false;
        const auto base=records_->GetGPUVirtualAddress();
        if(!base||base%64)return false;
        output.RayGenerationShaderRecord={base,32};output.MissShaderTable={base+64,32,32};
        output.HitGroupTable=hits;output.Width=count;output.Height=output.Depth=1;return true;
    }
};
}
