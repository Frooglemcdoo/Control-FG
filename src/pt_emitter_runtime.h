#pragma once
#include "pt_emitter_plan.h"
#include "pt_native_root_layout.h"
#include "pt_command_bindings.h"
#include <d3d12.h>
#include <mutex>
#include <wrl/client.h>
namespace PTEmitters {
using Microsoft::WRL::ComPtr;
// Ownership is identical to the private SBT: an Assets/recording/fence lease
// must retain Storage until neither recorded lists nor submitted work can use it.
class Storage {
    struct Entry {ComPtr<ID3D12Device> device;ComPtr<ID3D12Resource> resource;ComPtr<ID3D12DescriptorHeap> clearHeap;};
    std::unique_ptr<Entry> entry;
    inline static std::mutex mutex;
    inline static std::vector<std::unique_ptr<Entry>> idle;
    inline static UINT64 resident=0;
public:
    static constexpr UINT Capacity=65536,Bytes=16+80*Capacity;
    static constexpr UINT64 MaximumBytes=128ull*1024*1024;
    ID3D12Resource* Resource()const{return entry->resource.Get();}
    D3D12_CPU_DESCRIPTOR_HANDLE Uav()const{return entry->clearHeap->GetCPUDescriptorHandleForHeapStart();}
    static HRESULT Acquire(ID3D12Device* device,std::shared_ptr<Storage>& out){
        out.reset();if(!device)return E_INVALIDARG;
        auto result=std::make_shared<Storage>();std::lock_guard<std::mutex> lock(mutex);
        for(auto i=idle.begin();i!=idle.end();++i)if((*i)->device.Get()==device){result->entry=std::move(*i);idle.erase(i);break;}
        if(!result->entry){
            if(resident>MaximumBytes-Bytes)return E_OUTOFMEMORY;
            auto e=std::make_unique<Entry>();e->device=device;
            D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=Bytes;rd.Height=1;
            rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
            const auto hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&e->resource));
            if(FAILED(hr))return hr;
            D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;
            const auto heapHr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e->clearHeap));if(FAILED(heapHr))return heapHr;
            D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R32_TYPELESS;ud.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
            ud.Buffer.NumElements=Bytes/4;ud.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
            device->CreateUnorderedAccessView(e->resource.Get(),nullptr,&ud,e->clearHeap->GetCPUDescriptorHandleForHeapStart());
            resident+=Bytes;result->entry=std::move(e);
        }
        out=std::move(result);return S_OK;
    }
    ~Storage(){if(entry){std::lock_guard<std::mutex> lock(mutex);if(idle.size()<8)idle.push_back(std::move(entry));else resident-=Bytes;}}
};
class Builder {
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pipeline;
public:
    HRESULT Initialize(ID3D12Device* device,const PTNativeRoot::Layout& native,const void* code,SIZE_T size){
        if(!device||native.Type()!=PTNativeRoot::Kind::compute||!code||!size)return E_INVALIDARG;
        ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
        const auto& bytes=native.Serialized();auto hr=D3D12CreateVersionedRootSignatureDeserializer(bytes.data(),bytes.size(),IID_PPV_ARGS(&decoder));if(FAILED(hr))return hr;
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* converted=nullptr;hr=decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&converted);if(FAILED(hr))return hr;
        const auto& original=converted->Desc_1_0;if(original.NumParameters!=8)return E_INVALIDARG;
        std::array<D3D12_ROOT_PARAMETER,12> params{};std::copy(original.pParameters,original.pParameters+8,params.begin());
        for(UINT i=0;i<3;++i){params[8+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[8+i].Descriptor={i,4};}
        params[11].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[11].Constants={0,4,40};
        D3D12_ROOT_SIGNATURE_DESC desc=original;desc.NumParameters=12;desc.pParameters=params.data();
        ComPtr<ID3DBlob> serialized,error;hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1_0,&serialized,&error);if(FAILED(hr))return hr;
        hr=device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root));if(FAILED(hr))return hr;
        D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=root.Get();p.CS={code,size};return device->CreateComputePipelineState(&p,IID_PPV_ARGS(&pipeline));
    }
    static void Bind(ID3D12GraphicsCommandList* list,ID3D12RootSignature* root,const PTCommandBindings::Snapshot& bindings){
        list->SetComputeRootSignature(root);
        for(UINT i=0;i<8;++i){const auto& a=bindings.arguments[i];
            if(a.kind==PTCommandBindings::ArgumentKind::table)list->SetComputeRootDescriptorTable(i,{a.value});
            else if(a.kind==PTCommandBindings::ArgumentKind::constant)list->SetComputeRootConstantBufferView(i,a.value);
        }
    }
    bool Record(ID3D12GraphicsCommandList* list,const PTCommandBindings::Snapshot& bindings,
                D3D12_GPU_DESCRIPTOR_HANDLE table,D3D12_GPU_DESCRIPTOR_HANDLE gpuUav,D3D12_CPU_DESCRIPTOR_HANDLE cpuUav,
                Storage& storage,const std::vector<Geometry>& geometries){
        if(!list||!root||!pipeline||!table.ptr||!gpuUav.ptr||!cpuUav.ptr)return false;
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={storage.Resource(),0,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};list->ResourceBarrier(1,&barrier);
        const UINT zero[4]={};list->ClearUnorderedAccessViewUint(gpuUav,cpuUav,storage.Resource(),zero,0,nullptr);
        D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=storage.Resource();list->ResourceBarrier(1,&order);
        Bind(list,root.Get(),bindings);list->SetComputeRootDescriptorTable(0,table);list->SetPipelineState(pipeline.Get());
        for(const auto& g:geometries){
            list->SetComputeRootShaderResourceView(8,g.indices);list->SetComputeRootShaderResourceView(9,g.attributes);list->SetComputeRootShaderResourceView(10,g.positions);
            list->SetComputeRoot32BitConstants(11,40,g.constants.data(),0);list->Dispatch((g.constants[24]+63)/64,1,1);
        }
        barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&barrier);
        Bind(list,bindings.layout->Root(),bindings);list->SetPipelineState(bindings.pipeline.Get());return true;
    }
};
}
