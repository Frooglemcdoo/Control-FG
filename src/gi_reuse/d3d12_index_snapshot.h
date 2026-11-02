#pragma once
#include "index_snapshot_plan.h"
#include "d3d12_compute_bindings.h"
#include <d3dcompiler.h>
namespace gi_reuse {
// Exact uint16 snapshot without descriptor heap switches (which would also
// invalidate graphics tables). Source state and all lifetimes remain caller-owned.
// Standalone recorder; not enabled in native AS hooks by this header.
class IndexSnapshotPipeline {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
public:
    HRESULT Initialize(ID3D12Device* device,const void* shader,SIZE_T size) {
        root_.Reset();pipeline_.Reset();if(!device||!shader||!size)return E_INVALIDARG;
        D3D12_FEATURE_DATA_D3D12_OPTIONS4 options{};
        if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4,&options,sizeof(options)))||!options.Native16BitShaderOpsSupported)return DXGI_ERROR_UNSUPPORTED;
        D3D12_FEATURE_DATA_SHADER_MODEL model{D3D_SHADER_MODEL_6_2};
        if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&model,sizeof(model)))||model.HighestShaderModel<D3D_SHADER_MODEL_6_2)return DXGI_ERROR_UNSUPPORTED;
        D3D12_ROOT_PARAMETER params[3]{};
        params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;
        params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={0,0,3};
        D3D12_ROOT_SIGNATURE_DESC desc{3,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        Microsoft::WRL::ComPtr<ID3DBlob> blob,errors;
        auto hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);if(FAILED(hr))return hr;
        hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_));if(FAILED(hr))return hr;
        D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.CS={shader,size};
        return device->CreateComputePipelineState(&pso,IID_PPV_ARGS(&pipeline_));
    }
    bool Record(ID3D12GraphicsCommandList* list,ID3D12Resource* source,std::uint64_t offset,std::uint64_t bytes,
                ID3D12Resource* output,IndexSnapshotPlan& plan) const noexcept {
        plan={};if(!pipeline_||!list||!source||!output||source==output)return false;
        if(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE)return false;
        const auto src=source->GetDesc(),dst=output->GetDesc();IndexSnapshotPlan next;
        if(src.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||dst.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||
           (src.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)||!(dst.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||
           !IndexSnapshotPlan::Make(src.Width,offset,bytes,next)||next.copyBytes>dst.Width)return false;
        const auto address=source->GetGPUVirtualAddress(),target=output->GetGPUVirtualAddress();
        if(!address||!target||(address&3)||(target&3)||src.Width>UINT64_MAX-address||dst.Width>UINT64_MAX-target)return false;
        list->SetComputeRootSignature(root_.Get());list->SetPipelineState(pipeline_.Get());
        list->SetComputeRootShaderResourceView(0,address+next.sourceOffset);
        list->SetComputeRootUnorderedAccessView(1,target);
        const UINT constants[]={next.elements,next.prefixBytes,next.groupsX};list->SetComputeRoot32BitConstants(2,3,constants,0);
        list->Dispatch(next.groupsX,next.groupsY,1);plan=next;return true;
    }
    bool RecordRestoring(ID3D12GraphicsCommandList4* list,ID3D12Resource* source,std::uint64_t offset,std::uint64_t bytes,
                         ID3D12Resource* output,const ComputeBindingSnapshot& bindings,IndexSnapshotPlan& plan) const noexcept {
        plan={};if(!bindings.Ready()||!Record(list,source,offset,bytes,output,plan))return false;
        return bindings.Restore(list);
    }
};
}
