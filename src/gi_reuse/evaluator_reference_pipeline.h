#pragma once
#include <d3d12.h>

namespace gi_reuse {
// Preparation only. Caller must verify the exact native shader identity before
// selecting this PSO. Keep native bindings, node and flags; cached native driver
// bytecode cannot be used with the translated shader. No root/table mutation.
inline HRESULT CreateEvaluatorReferencePipeline(ID3D12Device* device,
    const D3D12_COMPUTE_PIPELINE_STATE_DESC& native,
    D3D12_SHADER_BYTECODE reference, ID3D12PipelineState** output) noexcept {
    if (!output) return E_POINTER;
    *output = nullptr;
    if (!device || !native.pRootSignature || !reference.pShaderBytecode ||
        !reference.BytecodeLength) return E_INVALIDARG;
    auto replacement = native;
    replacement.CS = reference;
    replacement.CachedPSO = {};
    return device->CreateComputePipelineState(&replacement, IID_PPV_ARGS(output));
}
}
