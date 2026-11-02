#pragma once
#include "raw_snapshot_plan.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace gi_reuse {
// Standalone recorder, deliberately not installed into native command-list hooks.
// Caller owns all resource/heap/PSO lifetimes, source NON_PIXEL_SHADER_RESOURCE
// state, output UAV state and synchronization, and saves/restores compute bindings.
class RawSnapshotPipeline {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
public:
    HRESULT Initialize(ID3D12Device* device, const void* shader, SIZE_T bytes) {
        root_.Reset(); pipeline_.Reset();
        if (!device || !shader || !bytes) return E_INVALIDARG;
        D3D12_ROOT_PARAMETER params[3]{};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[2].Constants = {0, 0, 1};
        D3D12_ROOT_SIGNATURE_DESC desc{3, params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE};
        Microsoft::WRL::ComPtr<ID3DBlob> blob, errors;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors);
        if (FAILED(hr)) return hr;
        hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_));
        if (FAILED(hr)) return hr;
        D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = root_.Get(); pso.CS = {shader, bytes};
        return device->CreateComputePipelineState(&pso, IID_PPV_ARGS(&pipeline_));
    }
    bool Record(ID3D12GraphicsCommandList* list, ID3D12Resource* source,
                std::uint64_t offset, std::uint64_t bytes, ID3D12Resource* output,
                RawSnapshotPlan& plan) const noexcept {
        plan = {};
        if (!pipeline_ || !list || !source || !output || source == output) return false;
        if (list->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT &&
            list->GetType() != D3D12_COMMAND_LIST_TYPE_COMPUTE) return false;
        const auto src = source->GetDesc(), dst = output->GetDesc();
        RawSnapshotPlan candidate;
        if (src.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            dst.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            (src.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) ||
            !(dst.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ||
            !RawSnapshotPlan::Make(src.Width, offset, bytes, candidate) ||
            candidate.copyBytes > dst.Width) return false;
        const auto address = source->GetGPUVirtualAddress(), target = output->GetGPUVirtualAddress();
        if (!address || !target || address % 4 || target % 4 ||
            src.Width > UINT64_MAX - address || dst.Width > UINT64_MAX - target) return false;
        // All validation precedes the first command. No source transitions,
        // descriptor heap changes, CPU waits, allocations or native state guesses.
        list->SetComputeRootSignature(root_.Get());
        list->SetPipelineState(pipeline_.Get());
        list->SetComputeRootShaderResourceView(0, address + candidate.sourceOffset);
        list->SetComputeRootUnorderedAccessView(1, target);
        list->SetComputeRoot32BitConstant(2, candidate.words, 0);
        list->Dispatch(candidate.groups, 1, 1);
        plan = candidate;
        return true;
    }
};
}
